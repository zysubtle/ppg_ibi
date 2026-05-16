#include "ppg_ibi.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 8192u
#define MAX_LINE_LEN 512u
#define HEADER_EXPECTED "timestamp_ms,ppg0,ppg1,ppg2,ppg3,allow_measure,ecg_ibi_ms"

typedef struct { uint32_t timestamp_ms; uint16_t ibi_ms; float confidence; ppg_ibi_state_t state; uint32_t flags; } pred_event_t;
typedef struct { uint32_t timestamp_ms; uint16_t ibi_ms; } truth_event_t;
typedef struct {
    uint32_t truth_count,pred_count,matched_count,miss_count,extra_count;
    float mae_ms,mean_error_ms,rmse_ms,max_abs_error_ms,p95_abs_error_ms;
} metrics_t;

static int cmp_float(const void *a,const void *b){float fa=*(const float*)a,fb=*(const float*)b;return (fa>fb)-(fa<fb);} 

static void compute_metrics(const pred_event_t *pred, uint32_t pred_n, const truth_event_t *truth, uint32_t truth_n, metrics_t *m){
    memset(m,0,sizeof(*m)); m->truth_count=truth_n; m->pred_count=pred_n;
    bool pred_used[MAX_EVENTS]={0}; float abs_err[MAX_EVENTS]={0}; float sum_abs=0.f,sum=0.f,sum_sq=0.f; uint32_t matched=0;
    for(uint32_t ti=0;ti<truth_n;ti++){
        int best=-1; uint32_t best_dt=0xffffffffu;
        for(uint32_t pi=0;pi<pred_n;pi++) if(!pred_used[pi]){
            uint32_t dt=(pred[pi].timestamp_ms>truth[ti].timestamp_ms)?(pred[pi].timestamp_ms-truth[ti].timestamp_ms):(truth[ti].timestamp_ms-pred[pi].timestamp_ms);
            if(dt<best_dt){best_dt=dt;best=(int)pi;}
        }
        if(best>=0){
            pred_used[(uint32_t)best]=true; float e=(float)pred[(uint32_t)best].ibi_ms-(float)truth[ti].ibi_ms; float ae=fabsf(e);
            abs_err[matched]=ae; sum_abs+=ae; sum+=e; sum_sq+=e*e; matched++;
        }
    }
    m->matched_count=matched; m->miss_count=(truth_n>matched)?(truth_n-matched):0u; m->extra_count=(pred_n>matched)?(pred_n-matched):0u;
    if(matched>0u){
        m->mae_ms=sum_abs/(float)matched; m->mean_error_ms=sum/(float)matched; m->rmse_ms=sqrtf(sum_sq/(float)matched);
        qsort(abs_err,matched,sizeof(float),cmp_float); m->max_abs_error_ms=abs_err[matched-1u]; m->p95_abs_error_ms=abs_err[(matched-1u)*95u/100u];
    }
}

static void print_metrics(const metrics_t *m){
    printf("truth_count=%u\n",m->truth_count); printf("pred_count=%u\n",m->pred_count); printf("matched_count=%u\n",m->matched_count);
    printf("miss_count=%u\n",m->miss_count); printf("extra_count=%u\n",m->extra_count); printf("mae_ms=%.3f\n",m->mae_ms);
    printf("mean_error_ms=%.3f\n",m->mean_error_ms); printf("rmse_ms=%.3f\n",m->rmse_ms); printf("max_abs_error_ms=%.3f\n",m->max_abs_error_ms);
    printf("p95_abs_error_ms=%.3f\n",m->p95_abs_error_ms);
}

static int run_one_synth(uint16_t bpm, metrics_t *out){
    ppg_ibi_context_t ctx; ppg_ibi_config_t cfg; ppg_ibi_get_default_config(&cfg); if(ppg_ibi_init(&ctx,&cfg)!=PPG_IBI_STATUS_OK) return 2;
    pred_event_t preds[MAX_EVENTS]; truth_event_t truths[MAX_EVENTS]; uint32_t pn=0,tn=0;
    const uint32_t dur_ms=12000u, dt=20u, period_ms=(uint32_t)(60000u/(uint32_t)bpm);
    for(uint32_t t=0;t<=dur_ms;t+=dt){
        float phase=2.0f*3.14159265f*((float)t/(float)period_ms); int32_t base=(int32_t)(1000.0f*sinf(phase));
        ppg_ibi_input_t in={0}; in.timestamp_ms=t; for(uint32_t c=0;c<4u;c++) in.ppg[c]=base; in.allow_measure=true;
        ppg_ibi_output_t o; if(ppg_ibi_process(&ctx,&in,&o)!=PPG_IBI_STATUS_OK) return 3;
        if(o.valid && pn<MAX_EVENTS){preds[pn].timestamp_ms=t; preds[pn].ibi_ms=o.ibi_ms; preds[pn].confidence=o.confidence; preds[pn].state=o.state; preds[pn].flags=o.flags; pn++;}
        if(t>0u && (t%period_ms)==0u && tn<MAX_EVENTS){truths[tn].timestamp_ms=t; truths[tn].ibi_ms=(uint16_t)period_ms; tn++;}
    }
    compute_metrics(preds,pn,truths,tn,out);
    return 0;
}

static int run_self_test(void){
    metrics_t m60,m75; int rc=run_one_synth(60u,&m60); if(rc!=0){printf("M5 self-test 60bpm: generation/process failed rc=%d\n",rc);return 1;}
    bool p60=(m60.matched_count>=3u && m60.mae_ms<=120.0f && m60.max_abs_error_ms<=200.0f);
    printf("M5 self-test 60bpm: matched=%u mae_ms=%.1f max_abs_error_ms=%.1f %s\n",m60.matched_count,m60.mae_ms,m60.max_abs_error_ms,p60?"pass":"fail");
    rc=run_one_synth(75u,&m75); if(rc!=0){printf("M5 self-test 75bpm: generation/process failed rc=%d\n",rc);return 1;}
    bool p75=(m75.matched_count>=3u && m75.mae_ms<=120.0f && m75.max_abs_error_ms<=200.0f);
    printf("M5 self-test 75bpm: matched=%u mae_ms=%.1f max_abs_error_ms=%.1f %s\n",m75.matched_count,m75.mae_ms,m75.max_abs_error_ms,p75?"pass":"fail");
    if(!p60){printf("M5 self-test failure reason 60bpm: need matched>=3 mae<=120 max_abs<=200\n");}
    if(!p75){printf("M5 self-test failure reason 75bpm: need matched>=3 mae<=120 max_abs<=200\n");}
    if(!(p60&&p75)) {
        return 1;
    }
    printf("M5 self-test passed.\n");
    return 0;
}

static int parse_csv(const char *path,pred_event_t *pred,uint32_t *pn,truth_event_t *truth,uint32_t *tn,uint32_t *invalid){
    FILE *fp=fopen(path,"r"); if(!fp){fprintf(stderr,"failed to open: %s\n",path);return 2;} char line[MAX_LINE_LEN];
    if(!fgets(line,sizeof(line),fp)){fclose(fp);fprintf(stderr,"empty input\n");return 2;} line[strcspn(line,"\r\n")]=0;
    if(strcmp(line,HEADER_EXPECTED)!=0){fclose(fp);fprintf(stderr,"invalid header\n");return 2;}
    ppg_ibi_context_t ctx; ppg_ibi_config_t cfg; ppg_ibi_get_default_config(&cfg); if(ppg_ibi_init(&ctx,&cfg)!=PPG_IBI_STATUS_OK){fclose(fp);return 2;}
    uint32_t lno=1; *pn=0; *tn=0; *invalid=0;
    while(fgets(line,sizeof(line),fp)){ lno++; line[strcspn(line,"\r\n")]=0;
        unsigned long ts; long p0,p1,p2,p3,allow,ecg; int n=sscanf(line,"%lu,%ld,%ld,%ld,%ld,%ld,%ld",&ts,&p0,&p1,&p2,&p3,&allow,&ecg);
        if(n!=7){(*invalid)++; fprintf(stderr,"line %u parse error\n",lno); continue;}
        if((allow!=0 && allow!=1) || ecg<0){fprintf(stderr,"line %u key field invalid\n",lno);fclose(fp);return 3;}
        ppg_ibi_input_t in={0}; in.timestamp_ms=(uint32_t)ts; in.ppg[0]=(int32_t)p0; in.ppg[1]=(int32_t)p1; in.ppg[2]=(int32_t)p2; in.ppg[3]=(int32_t)p3; in.allow_measure=(allow==1);
        ppg_ibi_output_t o; if(ppg_ibi_process(&ctx,&in,&o)!=PPG_IBI_STATUS_OK){fclose(fp);return 3;}
        if(o.valid && *pn<MAX_EVENTS){pred[*pn]=(pred_event_t){in.timestamp_ms,o.ibi_ms,o.confidence,o.state,o.flags}; (*pn)++;}
        if(ecg>0 && *tn<MAX_EVENTS){truth[*tn]=(truth_event_t){in.timestamp_ms,(uint16_t)ecg}; (*tn)++;}
    }
    fclose(fp); return 0;
}

int main(int argc,char **argv){
    if(argc==2 && strcmp(argv[1],"--self-test")==0) return run_self_test();
    if(argc!=2 && argc!=3){fprintf(stderr,"usage: %s --self-test | %s input.csv [output_prefix]\n",argv[0],argv[0]);return 2;}
    pred_event_t pred[MAX_EVENTS]; truth_event_t truth[MAX_EVENTS]; uint32_t pn=0,tn=0,invalid=0; int rc=parse_csv(argv[1],pred,&pn,truth,&tn,&invalid); if(rc!=0) return rc;
    metrics_t m; compute_metrics(pred,pn,truth,tn,&m); print_metrics(&m); printf("invalid_line_count=%u\n",invalid);
    if(argc==3){char p1[256],p2[256]; snprintf(p1,sizeof(p1),"%s_predictions.csv",argv[2]); snprintf(p2,sizeof(p2),"%s_metrics.csv",argv[2]);
        FILE *f1=fopen(p1,"w"); if(!f1) return 4; fprintf(f1,"timestamp_ms,ibi_ms,confidence,state,flags\n"); for(uint32_t i=0;i<pn;i++) fprintf(f1,"%u,%u,%.6f,%u,%u\n",pred[i].timestamp_ms,pred[i].ibi_ms,pred[i].confidence,(unsigned)pred[i].state,pred[i].flags); fclose(f1);
        FILE *f2=fopen(p2,"w"); if(!f2) return 4; fprintf(f2,"truth_count,pred_count,matched_count,miss_count,extra_count,mae_ms,mean_error_ms,rmse_ms,max_abs_error_ms,p95_abs_error_ms,invalid_line_count\n");
        fprintf(f2,"%u,%u,%u,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%u\n",m.truth_count,m.pred_count,m.matched_count,m.miss_count,m.extra_count,m.mae_ms,m.mean_error_ms,m.rmse_ms,m.max_abs_error_ms,m.p95_abs_error_ms,invalid); fclose(f2);
    }
    return 0;
}
