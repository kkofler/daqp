#include "hierarchical.h"
#include "types.h"

#define DAQP_HIQP_SOFT ((c_float)1e-8)

int daqp_hiqp(DAQPWorkspace *work){
    int i,j,id;
    int start,end;
    int iterations = 0;
    int exitflag=0;
    c_float w;
    start=0;
    work->settings->rho_soft = DAQP_HIQP_SOFT;
    for(i =0; i < work->hier->nh; i++){
        // initialize current level
        end=work->hier->break_points[i];
        work->m = end;

        // Solve LDP
        exitflag = daqp_ldp(work);
        iterations+=work->iterations;
        if(exitflag < 0) break;

        // Perturb rhs with slacks in level 
        for(j=0; j<work->n_active;j++){
            id=work->WS[j];
            if(IS_SOFT(id)){ 
                w = work->lam_star[j]*work->settings->rho_soft*1.025;
                if(-1e-4 < w &&  w < 1e-4) continue; // Too small
                if(IS_LOWER(id))
                    work->dlower[id]+=w;
                else
                    work->dupper[id]+=w;
            }
        }

        // Make constraints in current level hard
        for(j=start; j<end;j++) SET_HARD(j);
        
        // find first active constraint in current level 
        for(j=0;j < work->n_active; j++)
            if(work->WS[j]>=start) break;
        // reactive constraint in level current (to addresss soft->hard)
        // TODO: can factorization be directly reused?
        int n_active_old = (work->n_active < work->n) ? work->n_active : work->n;
        for(int jj=n_active_old; jj < work->n_active ;jj++) SET_INACTIVE(work->WS[jj]);
        work->n_active =j;
        work->reuse_ind=j;
        work->sing_ind = EMPTY_IND;
        for(; j<n_active_old ;j++){
            add_constraint(work,work->WS[j],work->lam_star[j]);
            // Abort reactivation if WS becomes overdtermined
            if(work->sing_ind != EMPTY_IND){
                remove_constraint(work,j);
                work->sing_ind = EMPTY_IND;
                for(; j<n_active_old ;j++) SET_INACTIVE(work->WS[j]);
                break;
            }
        }

        // Move up hierarchy
        start = end;
    }
    work->iterations = iterations; // Append total number of iterations
    return exitflag;
}
