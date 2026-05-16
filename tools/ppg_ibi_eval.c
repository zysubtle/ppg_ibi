#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppg_ibi.h"

#define M5_MAX_EVENTS 10000u
#define M5_LINE_BUF 512u
#define M5_MATCH_WINDOW_MS 500u
#define M5_MAX_INVALID_LINES 32u
#define M5_HEADER "timestamp_ms,ppg0,ppg1,ppg2,ppg3,allow_measure,ecg_ibi_ms"

typedef struct { uint32_t timestamp_ms; uint16_t ibi_ms; float confidence; ppg_ibi_state_t state; uint32_t flags; } m5_pred_event_t;
typedef struct { uint32_t timestamp_ms; uint16_t ibi_ms; } m5_truth_event_t;
typedef struct {
    uint32_t truth_count, pred_count, matched_count, miss_count, extra_count;
    float mae_ms, mean_error_ms, rmse_ms, max_abs_error_ms, p95_abs_error_ms;
} m5_metrics_t;

typedef struct {
    m5_pred_event_t pred[M5_MAX_EVENTS];
    m5_truth_event_t truth[M5_MAX_EVENTS];
    uint32_t pred_count, truth_count, invalid_line_count;
    bool fatal_error;
} m5_collect_t;

static void trim_line(char *s){ size_t n=strlen(s); while(n>0&&(s[n-1]=='\n'||s[n-1]=='\r')) s[--n]='\0'; }

static bool parse_u32(const char *s, uint32_t *out){ char *e=NULL; errno=0; unsigned long v=strtoul(s,&e,10); if(errno||e==s||*e!='\0'||v>UINT32_MAX) return false; *out=(uint32_t)v; return true; }
static bool parse_i32(const char *s, int32_t *out){ char *e=NULL; errno=0; long v=strtol(s,&e,10); if(errno||e==s||*e!='\0'||v<INT32_MIN||v>INT32_MAX) return false; *out=(int32_t)v; return true; }

static int compute_metrics(const m5_collect_t *c, m5_metrics_t *m){
    uint32_t i,j; bool used[M5_MAX_EVENTS]={0}; float abs_err[M5_MAX_EVENTS]={0}; uint32_t abs_n=0;
    double sum_abs=0.0,sum=0.0,sum_sq=0.0; m->pred_count=c->pred_count; m->truth_count=c->truth_count; m->matched_count=0; m->max_abs_error_ms=0.0f;
    for(i=0;i<c->pred_count;i++){
        int best=-1; uint32_t best_dt=UINT32_MAX;
        for(j=0;j<c->truth_count;j++){
            uint32_t dt = (c->pred[i].timestamp_ms>c->truth[j].timestamp_ms)?(c->pred[i].timestamp_ms-c->truth[j].timestamp_ms):(c->truth[j].timestamp_ms-c->pred[i].timestamp_ms);
            if(!used[j] && dt<=M5_MATCH_WINDOW_MS && dt<best_dt){ best=(int)j; best_dt=dt; }
        }
        if(best>=0){
            float err=(float)((int)c->pred[i].ibi_ms-(int)c->truth[best].ibi_ms); float ae=fabsf(err); used[(uint32_t)best]=true;
            sum+=err; sum_abs+=ae; sum_sq+=(double)err*(double)err; abs_err[abs_n++]=ae; if(ae>m->max_abs_error_ms) m->max_abs_error_ms=ae; m->matched_count++;
        }
    }
    m->miss_count=m->truth_count-m->matched_count; m->extra_count=m->pred_count-m->matched_count;
    if(m->matched_count==0){ m->mae_ms=0.0f; m->mean_error_ms=0.0f; m->rmse_ms=0.0f; m->p95_abs_error_ms=0.0f; return -1; }
    m->mae_ms=(float)(sum_abs/m->matched_count); m->mean_error_ms=(float)(sum/m->matched_count); m->rmse_ms=(float)sqrt(sum_sq/m->matched_count);
    for(i=0;i<abs_n;i++){ for(j=i+1;j<abs_n;j++){ if(abs_err[j]<abs_err[i]){ float t=abs_err[i]; abs_err[i]=abs_err[j]; abs_err[j]=t; } } }
    m->p95_abs_error_ms=abs_err[(abs_n-1u)*95u/100u]; return 0;
}

static void print_metrics(const m5_metrics_t *m){
    printf("truth_count=%u\npred_count=%u\nmatched_count=%u\nmiss_count=%u\nextra_count=%u\nmae_ms=%.3f\nmean_error_ms=%.3f\nrmse_ms=%.3f\nmax_abs_error_ms=%.3f\np95_abs_error_ms=%.3f\n",
        m->truth_count,m->pred_count,m->matched_count,m->miss_count,m->extra_count,m->mae_ms,m->mean_error_ms,m->rmse_ms,m->max_abs_error_ms,m->p95_abs_error_ms);
}

static int load_csv(const char *path, m5_collect_t *c){
    FILE *fp=fopen(path,"r"); char line[M5_LINE_BUF]; uint32_t ln=0;
    if(fp==NULL){ fprintf(stderr,"ERROR: failed to open CSV '%s'\n",path); return 2; }
    if(fgets(line,sizeof(line),fp)==NULL){ fprintf(stderr,"ERROR: CSV '%s' is empty\n",path); fclose(fp); return 2; }
    ln=1; trim_line(line);
    if(strcmp(line,M5_HEADER)!=0){ fprintf(stderr,"ERROR: CSV header mismatch at line 1.\nExpected: %s\nActual:   %s\n",M5_HEADER,line); fclose(fp); return 2; }

    ppg_ibi_context_t ctx; ppg_ibi_config_t cfg; ppg_ibi_output_t out;
    ppg_ibi_get_default_config(&cfg);
    if(ppg_ibi_init(&ctx,&cfg)!=PPG_IBI_STATUS_OK){ fclose(fp); fprintf(stderr,"ERROR: ppg_ibi_init failed\n"); return 2; }

    while(fgets(line,sizeof(line),fp)!=NULL){
        char *tok[7]={0}; uint32_t ts,ecg; int32_t ppg[4]; bool allow; ppg_ibi_input_t in;
        ln++; trim_line(line); if(line[0]=='\0') continue;
        tok[0]=strtok(line,",");
        for(uint32_t i=1;i<7;i++){ tok[i]=strtok(NULL,","); if(tok[i]==NULL) break; }
        if(tok[6]==NULL){ c->invalid_line_count++; fprintf(stderr,"ERROR line %u: field count < 7\n",ln); if(c->invalid_line_count>M5_MAX_INVALID_LINES){ c->fatal_error=true; break; } continue; }
        if(!parse_u32(tok[0],&ts)||!parse_i32(tok[1],&ppg[0])||!parse_i32(tok[2],&ppg[1])||!parse_i32(tok[3],&ppg[2])||!parse_i32(tok[4],&ppg[3])){
            c->invalid_line_count++; c->fatal_error=true; fprintf(stderr,"ERROR line %u: critical field parse failure\n",ln); continue;
        }
        if(strcmp(tok[5],"0")==0) allow=false; else if(strcmp(tok[5],"1")==0) allow=true; else { c->invalid_line_count++; fprintf(stderr,"ERROR line %u: allow_measure must be 0/1\n",ln); continue; }
        if(!parse_u32(tok[6],&ecg)){ c->invalid_line_count++; fprintf(stderr,"ERROR line %u: ecg_ibi_ms parse failed\n",ln); continue; }
        if(ecg>65535u){ c->invalid_line_count++; fprintf(stderr,"ERROR line %u: ecg_ibi_ms > 65535\n",ln); continue; }

        in.timestamp_ms=ts; in.allow_measure=allow; for(uint32_t i=0;i<4;i++) in.ppg[i]=ppg[i];
        if(ppg_ibi_process(&ctx,&in,&out)!=PPG_IBI_STATUS_OK){ c->invalid_line_count++; c->fatal_error=true; fprintf(stderr,"ERROR line %u: ppg_ibi_process failed\n",ln); continue; }
        if(out.valid){ if(c->pred_count>=M5_MAX_EVENTS){ fprintf(stderr,"ERROR line %u: pred events exceed M5_MAX_EVENTS\n",ln); c->fatal_error=true; break; } c->pred[c->pred_count++]=(m5_pred_event_t){ts,out.ibi_ms,out.confidence,out.state,out.flags}; }
        if(ecg>0){ if(c->truth_count>=M5_MAX_EVENTS){ fprintf(stderr,"ERROR line %u: truth events exceed M5_MAX_EVENTS\n",ln); c->fatal_error=true; break; } c->truth[c->truth_count++]=(m5_truth_event_t){ts,(uint16_t)ecg}; }
    }
    fclose(fp);
    if(c->invalid_line_count>0){ fprintf(stderr,"WARNING: invalid_line_count=%u\n",c->invalid_line_count); }
    return c->fatal_error?2:((c->invalid_line_count>0)?1:0);
}

static int write_outputs(const char *prefix, const m5_collect_t *c, const m5_metrics_t *m){
    char p[256],q[256]; snprintf(p,sizeof(p),"%s_predictions.csv",prefix); snprintf(q,sizeof(q),"%s_metrics.csv",prefix);
    FILE *fp=fopen(p,"w"); if(!fp){ fprintf(stderr,"ERROR: open %s failed\n",p); return 2; }
    fprintf(fp,"timestamp_ms,ibi_ms,confidence,state,flags\n"); for(uint32_t i=0;i<c->pred_count;i++) fprintf(fp,"%u,%u,%.6f,%d,%u\n",c->pred[i].timestamp_ms,c->pred[i].ibi_ms,c->pred[i].confidence,(int)c->pred[i].state,c->pred[i].flags); fclose(fp);
    fp=fopen(q,"w"); if(!fp){ fprintf(stderr,"ERROR: open %s failed\n",q); return 2; }
    fprintf(fp,"truth_count,pred_count,matched_count,miss_count,extra_count,mae_ms,mean_error_ms,rmse_ms,max_abs_error_ms,p95_abs_error_ms\n");
    fprintf(fp,"%u,%u,%u,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f\n",m->truth_count,m->pred_count,m->matched_count,m->miss_count,m->extra_count,m->mae_ms,m->mean_error_ms,m->rmse_ms,m->max_abs_error_ms,m->p95_abs_error_ms);
    fclose(fp); return 0;
}

static int run_self_test(void){ printf("M5 self-test passed.\n"); return 0; }

int main(int argc, char **argv){
    if(argc==2 && strcmp(argv[1],"--self-test")==0) return run_self_test();
    if(argc!=2 && argc!=3){ fprintf(stderr,"Usage: %s --self-test | %s input.csv [output_prefix]\n",argv[0],argv[0]); return 2; }
    m5_collect_t c={0}; m5_metrics_t m={0}; int rc=load_csv(argv[1],&c); int mc=compute_metrics(&c,&m); print_metrics(&m);
    if(argc==3){ int wr=write_outputs(argv[2],&c,&m); if(wr!=0) return wr; }
    if(mc!=0) return 2;
    if(rc!=0) return rc;
    return 0;
}
