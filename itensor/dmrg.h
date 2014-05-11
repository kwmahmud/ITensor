//
// Distributed under the ITensor Library License, Version 1.1.
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_DMRG_H
#define __ITENSOR_DMRG_H

#include "eigensolver.h"
#include "localmposet.h"
#include "localmpo_mps.h"
#include "sweeps.h"
#include "DMRGObserver.h"

#define Cout std::cout
#define Endl std::endl
#define Format boost::format

//
// Available DMRG methods:
//

//
//DMRG with an MPO
//
template <class Tensor>
Real
dmrg(MPSt<Tensor>& psi, 
     const MPOt<Tensor>& H, 
     const Sweeps& sweeps,
     const OptSet& opts = Global::opts())
    {
    LocalMPO<Tensor> PH(H,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,opts);
    return energy;
    }

//
//DMRG with an MPO and custom Observer
//
template <class Tensor>
Real
dmrg(MPSt<Tensor>& psi, 
     const MPOt<Tensor>& H, 
     const Sweeps& sweeps, 
     Observer& obs,
     const OptSet& opts = Global::opts())
    {
    LocalMPO<Tensor> PH(H,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,obs,opts);
    return energy;
    }

//
//DMRG with an MPO and boundary tensors LH, RH
// LH - H1 - H2 - ... - HN - RH
//(ok if one or both of LH, RH default constructed)
//
template <class Tensor>
Real
dmrg(MPSt<Tensor>& psi, 
     const MPOt<Tensor>& H, 
     const Tensor& LH, const Tensor& RH,
     const Sweeps& sweeps,
     const OptSet& opts = Global::opts())
    {
    LocalMPO<Tensor> PH(H,LH,RH,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,opts);
    return energy;
    }

//
//DMRG with an MPO and boundary tensors LH, RH
//and a custom observer
//
template <class Tensor>
Real
dmrg(MPSt<Tensor>& psi, 
     const MPOt<Tensor>& H, 
     const Tensor& LH, const Tensor& RH,
     const Sweeps& sweeps, 
     Observer& obs,
     const OptSet& opts = Global::opts())
    {
    LocalMPO<Tensor> PH(H,LH,RH,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,obs,opts);
    return energy;
    }

//
//DMRG with a set of MPOs (lazily summed)
//(H vector is 0-indexed)
//
template <class Tensor>
Real
dmrg(MPSt<Tensor>& psi, 
     const std::vector<MPOt<Tensor> >& Hset, 
     const Sweeps& sweeps,
     const OptSet& opts = Global::opts())
    {
    LocalMPOSet<Tensor> PH(Hset,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,opts);
    return energy;
    }

//
//DMRG with a set of MPOs and a custom Observer
//(H vector is 0-indexed)
//
template <class Tensor>
Real 
dmrg(MPSt<Tensor>& psi, 
     const std::vector<MPOt<Tensor> >& Hset, 
     const Sweeps& sweeps, 
     Observer& obs,
     const OptSet& opts = Global::opts())
    {
    LocalMPOSet<Tensor> PH(Hset,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,obs,opts);
    return energy;
    }

//
//DMRG with a single Hamiltonian MPO and a set of 
//MPS to orthogonalize against
//(psis vector is 0-indexed)
//Options recognized:
// Weight - real number w > 0; calling dmrg(psi,H,psis,sweeps,Opt("Weight",w))
//          sets the effective Hamiltonian to be
//          H + w * (|0><0| + |1><1| + ...) where |0> = psis[0], |1> = psis[1]
//          etc.
//
template <class Tensor>
Real
dmrg(MPSt<Tensor>& psi, 
     const MPOt<Tensor>& H, 
     const std::vector<MPSt<Tensor> >& psis, 
     const Sweeps& sweeps, 
     const OptSet& opts = Global::opts())
    {
    LocalMPO_MPS<Tensor> PH(H,psis,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,opts);
    return energy;
    }

//
//DMRG with a single Hamiltonian MPO, 
//a set of MPS to orthogonalize against, 
//and a custom Observer.
//(psis vector is 0-indexed)
//Options recognized:
// Weight - real number w > 0; calling dmrg(psi,H,psis,sweeps,Opt("Weight",w))
//          sets the effective Hamiltonian to be
//          H + w * (|0><0| + |1><1| + ...) where |0> = psis[0], |1> = psis[1]
//          etc.
//
template <class Tensor>
Real
dmrg(MPSt<Tensor>& psi, 
     const MPOt<Tensor>& H, 
     const std::vector<MPSt<Tensor> >& psis, 
     const Sweeps& sweeps, Observer& obs, 
     const OptSet& opts = Global::opts())
    {
    LocalMPO_MPS<Tensor> PH(H,psis,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,obs,opts);
    return energy;
    }



//
// DMRGWorker
//

template <class Tensor, class LocalOpT>
Real inline
DMRGWorker(MPSt<Tensor>& psi,
           LocalOpT& PH,
           const Sweeps& sweeps,
           const OptSet& opts = Global::opts())
    {
    //Default Observer type is DMRGObserver
    DMRGObserver<Tensor> obs(psi,opts);
    Real energy = DMRGWorker(psi,PH,sweeps,obs,opts);
    return energy;
    }

template <class Tensor, class LocalOpT>
Real
DMRGWorker(MPSt<Tensor>& psi,
           LocalOpT& PH,
           const Sweeps& sweeps,
           Observer& obs,
           OptSet opts = Global::opts())
    {
    const bool quiet = opts.getBool("Quiet",false);
    const int debug_level = opts.getInt("DebugLevel",(quiet ? 0 : 1));

    const int N = psi.N();
    Real energy = NAN;

    psi.position(1);

    opts.add("DebugLevel",debug_level);
    opts.add("DoNormalize",true);
    
    for(int sw = 1; sw <= sweeps.nsweep(); ++sw)
        {
        opts.add("Sweep",sw);
        opts.add("Cutoff",sweeps.cutoff(sw));
        opts.add("Minm",sweeps.minm(sw));
        opts.add("Maxm",sweeps.maxm(sw));
        opts.add("Noise",sweeps.noise(sw));
        opts.add("MaxIter",sweeps.niter(sw));

        if(!PH.doWrite() &&
            opts.defined("WriteM") &&
            sweeps.maxm(sw) >= opts.getInt("WriteM"))
            {
            std::string write_dir = opts.getString("WriteDir","./");

            if(!quiet)
                {
                Cout << "\nTurning on write to disk, write_dir = " 
                     << write_dir << Endl;
                }

            psi.doWrite(true);
            PH.doWrite(true);
            }

        for(int b = 1, ha = 1; ha <= 2; sweepnext(b,ha,N))
            {
            if(!quiet)
                {
                Cout << Format("Sweep=%d, HS=%d, Bond=(%d,%d)") 
                        % sw % ha % b % (b+1) << Endl;
                }

            PH.position(b,psi);

            Tensor phi = psi.A(b)*psi.A(b+1);

            energy = davidson(PH,phi,opts);
            
            psi.svdBond(b,phi,(ha==1?Fromleft:Fromright),PH,opts);

            if(!quiet)
                { 
                Cout << 
                    Format("    Truncated to Cutoff=%.1E, Min_m=%d, Max_m=%d") 
                        % sweeps.cutoff(sw) 
                        % sweeps.minm(sw) 
                        % sweeps.maxm(sw) 
                        << Endl;
                Cout << Format("    Trunc. err=%.1E, States kept=%s")
                        % psi.spectrum(b).truncerr() 
                        % showm(psi.LinkInd(b)) 
                        << Endl;
                }

            opts.add("AtBond",b);
            opts.add("HalfSweep",ha);
            opts.add("Energy",energy); 

            obs.measure(opts);

            } //for loop over b

        if(obs.checkDone(opts)) break;
    
        } //for loop over sw
    
    psi.normalize();

    return energy;
    }


#undef Cout
#undef Endl
#undef Format

#endif
