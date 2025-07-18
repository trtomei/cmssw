/////////////////////////////// BToTrkLLBuilder ///////////////////////////////
/// original authors: G Karathanasis (CERN),  G Melachroinos (NKUA)
// takes selected track collection and a dilepton collection and produces B
// moth// - ers using a three-track vertex

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CommonTools/Statistics/interface/ChiSquaredProbability.h"
#include "CommonTools/Utils/interface/StringCutObjectSelector.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/CompositeCandidate.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "KinVtxFitter.h"
#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "helper.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

class BToTrkLLBuilder : public edm::global::EDProducer<> {
  // perhaps we need better structure here (begin run etc)
public:
  typedef std::vector<reco::TransientTrack> TransientTrackCollection;

  explicit BToTrkLLBuilder(const edm::ParameterSet &cfg)
      : bFieldToken_{esConsumes<MagneticField, IdealMagneticFieldRecord>()},
        pre_vtx_selection_{cfg.getParameter<std::string>("preVtxSelection")},
        post_vtx_selection_{cfg.getParameter<std::string>("postVtxSelection")},
        dileptons_{consumes<pat::CompositeCandidateCollection>(cfg.getParameter<edm::InputTag>("dileptons"))},
        leptons_ttracks_{consumes<TransientTrackCollection>(cfg.getParameter<edm::InputTag>("leptonTransientTracks"))},
        kaons_{consumes<pat::CompositeCandidateCollection>(cfg.getParameter<edm::InputTag>("kaons"))},
        kaons_ttracks_{consumes<TransientTrackCollection>(cfg.getParameter<edm::InputTag>("kaonsTransientTracks"))},
        track_mass_{cfg.getParameter<double>("trackMass")},
        pu_tracks_(consumes<pat::CompositeCandidateCollection>(cfg.getParameter<edm::InputTag>("PUtracks"))),
        beamspot_{consumes<reco::BeamSpot>(cfg.getParameter<edm::InputTag>("beamSpot"))},
        dilepton_constraint_{cfg.getParameter<bool>("dileptonMassContraint")} {
    produces<pat::CompositeCandidateCollection>();
  }

  ~BToTrkLLBuilder() override {}

  void produce(edm::StreamID, edm::Event &, const edm::EventSetup &) const override;

private:
  const edm::ESGetToken<MagneticField, IdealMagneticFieldRecord> bFieldToken_;

  // selections
  const StringCutObjectSelector<pat::CompositeCandidate> pre_vtx_selection_;
  const StringCutObjectSelector<pat::CompositeCandidate> post_vtx_selection_;

  // inputs
  const edm::EDGetTokenT<pat::CompositeCandidateCollection> dileptons_;
  const edm::EDGetTokenT<TransientTrackCollection> leptons_ttracks_;
  const edm::EDGetTokenT<pat::CompositeCandidateCollection> kaons_;
  const edm::EDGetTokenT<TransientTrackCollection> kaons_ttracks_;
  const double track_mass_;
  const edm::EDGetTokenT<pat::CompositeCandidateCollection> pu_tracks_;
  const edm::EDGetTokenT<reco::BeamSpot> beamspot_;
  const bool dilepton_constraint_;
};

void BToTrkLLBuilder::produce(edm::StreamID, edm::Event &evt, edm::EventSetup const &iSetup) const {
  // input
  edm::Handle<pat::CompositeCandidateCollection> dileptons;
  evt.getByToken(dileptons_, dileptons);
  edm::Handle<TransientTrackCollection> leptons_ttracks;
  evt.getByToken(leptons_ttracks_, leptons_ttracks);

  edm::Handle<pat::CompositeCandidateCollection> kaons;
  evt.getByToken(kaons_, kaons);
  edm::Handle<TransientTrackCollection> kaons_ttracks;
  evt.getByToken(kaons_ttracks_, kaons_ttracks);

  edm::Handle<pat::CompositeCandidateCollection> pu_tracks;
  evt.getByToken(pu_tracks_, pu_tracks);

  edm::Handle<reco::BeamSpot> beamspot;
  evt.getByToken(beamspot_, beamspot);

  edm::ESHandle<MagneticField> fieldHandle;
  const auto &bField = iSetup.getData(bFieldToken_);
  AnalyticalImpactPointExtrapolator extrapolator(&bField);

  // output
  std::unique_ptr<pat::CompositeCandidateCollection> ret_val(new pat::CompositeCandidateCollection());

  for (size_t k_idx = 0; k_idx < kaons->size(); ++k_idx) {
    edm::Ptr<pat::CompositeCandidate> k_ptr(kaons, k_idx);

    math::PtEtaPhiMLorentzVector k_p4(k_ptr->pt(), k_ptr->eta(), k_ptr->phi(), bph::K_MASS);

    for (size_t ll_idx = 0; ll_idx < dileptons->size(); ++ll_idx) {
      edm::Ptr<pat::CompositeCandidate> ll_prt(dileptons, ll_idx);
      edm::Ptr<reco::Candidate> l1_ptr = ll_prt->userCand("l1");
      edm::Ptr<reco::Candidate> l2_ptr = ll_prt->userCand("l2");
      int l1_idx = ll_prt->userInt("l1_idx");
      int l2_idx = ll_prt->userInt("l2_idx");

      pat::CompositeCandidate cand;
      cand.setP4(ll_prt->p4() + k_p4);
      cand.setCharge(ll_prt->charge() + k_ptr->charge());
      // Use UserCands as they should not use memory but keep the Ptr itself
      // Put the lepton passing the corresponding selection
      cand.addUserCand("l1", l1_ptr);
      cand.addUserCand("l2", l2_ptr);
      cand.addUserCand("trk", k_ptr);
      cand.addUserCand("dilepton", ll_prt);

      cand.addUserInt("l1_idx", l1_idx);
      cand.addUserInt("l2_idx", l2_idx);
      cand.addUserInt("trk_idx", k_idx);
      cand.addUserInt("ll_idx", ll_idx);

      auto dr_info = bph::min_max_dr({l1_ptr, l2_ptr, k_ptr});
      cand.addUserFloat("min_dr", dr_info.first);
      cand.addUserFloat("max_dr", dr_info.second);

      if (!pre_vtx_selection_(cand))
        continue;

      KinVtxFitter fitter;
      try {
        fitter = KinVtxFitter({leptons_ttracks->at(l1_idx), leptons_ttracks->at(l2_idx), kaons_ttracks->at(k_idx)},
                              {l1_ptr->mass(), l2_ptr->mass(), bph::K_MASS},
                              {bph::LEP_SIGMA, bph::LEP_SIGMA, bph::K_SIGMA});
      } catch (const VertexException &e) {
        edm::LogWarning("KinematicFit") << "BToKLL: Skipping candidate due to fit failure: " << e.what();
        continue;
      }

      if (!fitter.success())
        continue;  // hardcoded, but do we need otherwise?
      cand.setVertex(reco::Candidate::Point(fitter.fitted_vtx().x(), fitter.fitted_vtx().y(), fitter.fitted_vtx().z()));
      cand.addUserInt("sv_OK", fitter.success());
      cand.addUserFloat("sv_chi2", fitter.chi2());
      cand.addUserFloat("sv_ndof", fitter.dof());  // float??
      cand.addUserFloat("sv_prob", fitter.prob());
      cand.addUserFloat("fitted_mll", (fitter.daughter_p4(0) + fitter.daughter_p4(1)).mass());
      auto fit_p4 = fitter.fitted_p4();
      cand.addUserFloat("fitted_pt", fit_p4.pt());
      cand.addUserFloat("fitted_eta", fit_p4.eta());
      cand.addUserFloat("fitted_phi", fit_p4.phi());
      cand.addUserFloat("fitted_mass", fitter.fitted_candidate().mass());
      cand.addUserFloat("fitted_massErr", sqrt(fitter.fitted_candidate().kinematicParametersError().matrix()(6, 6)));

      cand.addUserFloat("cos_theta_2D", bph::cos_theta_2D(fitter, *beamspot, cand.p4()));
      cand.addUserFloat("fitted_cos_theta_2D", bph::cos_theta_2D(fitter, *beamspot, fit_p4));

      auto lxy = bph::l_xy(fitter, *beamspot);
      cand.addUserFloat("l_xy", lxy.value());
      cand.addUserFloat("l_xy_unc", lxy.error());
      // track impact parameter from SV
      TrajectoryStateOnSurface tsos =
          extrapolator.extrapolate(kaons_ttracks->at(k_idx).impactPointState(), fitter.fitted_vtx());
      std::pair<bool, Measurement1D> cur2DIP =
          bph::signedTransverseImpactParameter(tsos, fitter.fitted_refvtx(), *beamspot);
      cand.addUserFloat("k_svip2d", cur2DIP.second.value());
      cand.addUserFloat("k_svip2d_err", cur2DIP.second.error());

      if (!post_vtx_selection_(cand))
        continue;

      const reco::BeamSpot &beamSpot = *beamspot;
      TrajectoryStateClosestToPoint theDCAXBS = fitter.fitted_candidate_ttrk().trajectoryStateClosestToPoint(
          GlobalPoint(beamSpot.position().x(), beamSpot.position().y(), beamSpot.position().z()));
      double DCAB0BS = -99.;
      double DCAB0BSErr = -99.;

      if (theDCAXBS.isValid() == true) {
        DCAB0BS = theDCAXBS.perigeeParameters().transverseImpactParameter();
        DCAB0BSErr = theDCAXBS.perigeeError().transverseImpactParameterError();
      }
      cand.addUserFloat("dca", DCAB0BS);
      cand.addUserFloat("dcaErr", DCAB0BSErr);

      cand.addUserFloat("vtx_x", cand.vx());
      cand.addUserFloat("vtx_y", cand.vy());
      cand.addUserFloat("vtx_z", cand.vz());

      const auto &covMatrix = fitter.fitted_vtx_uncertainty();
      cand.addUserFloat("vtx_cxx", covMatrix.cxx());
      cand.addUserFloat("vtx_cyy", covMatrix.cyy());
      cand.addUserFloat("vtx_czz", covMatrix.czz());
      cand.addUserFloat("vtx_cyx", covMatrix.cyx());
      cand.addUserFloat("vtx_czx", covMatrix.czx());
      cand.addUserFloat("vtx_czy", covMatrix.czy());

      // refitted daughters (leptons/tracks)
      std::vector<std::string> dnames{"l1", "l2", "trk"};

      for (size_t idaughter = 0; idaughter < dnames.size(); idaughter++) {
        cand.addUserFloat("fitted_" + dnames[idaughter] + "_pt", fitter.daughter_p4(idaughter).pt());

        cand.addUserFloat("fitted_" + dnames[idaughter] + "_eta", fitter.daughter_p4(idaughter).eta());

        cand.addUserFloat("fitted_" + dnames[idaughter] + "_phi", fitter.daughter_p4(idaughter).phi());
      }

      // compute isolation
      std::vector<float> isos = bph::TrackerIsolation(pu_tracks, cand, dnames);
      for (size_t idaughter = 0; idaughter < dnames.size(); idaughter++) {
        cand.addUserFloat(dnames[idaughter] + "_iso04", isos[idaughter]);
      }

      float constraint_sv_prob = -9;
      float constraint_pt = -9;
      float constraint_eta = -9;
      float constraint_phi = -9;
      float constraint_mass = -9;
      float constraint_massErr = -9;
      float constraint_mll = -9;

      const double dilepton_mass = ll_prt->userFloat("fitted_mass");
      const double jpsi_bin[2] = {2.8, 3.35};
      const double psi2s_bin[2] = {3.45, 3.85};

      if (dilepton_constraint_ && ((dilepton_mass > jpsi_bin[0] && dilepton_mass < jpsi_bin[1]) ||
                                   (dilepton_mass > psi2s_bin[0] && dilepton_mass < psi2s_bin[1]))) {
        ParticleMass JPsi_mass = 3.0969;   // Jpsi mass 3.096900±0.000006
        ParticleMass Psi2S_mass = 3.6861;  // Psi2S mass 3.6861093±0.0000034
        ParticleMass mass_constraint = (dilepton_mass < jpsi_bin[1]) ? JPsi_mass : Psi2S_mass;

        // Mass constraint is applied to the first two particles in the
        // "particles" vector Make sure that the first two particles are the
        // ones you want to constrain
        KinVtxFitter constraint_fitter;
        try {
          constraint_fitter =
              KinVtxFitter({leptons_ttracks->at(l1_idx), leptons_ttracks->at(l2_idx), kaons_ttracks->at(k_idx)},
                           {l1_ptr->mass(), l2_ptr->mass(), bph::K_MASS},
                           {bph::LEP_SIGMA, bph::LEP_SIGMA, bph::K_SIGMA},
                           mass_constraint);
        } catch (const VertexException &e) {
          edm::LogWarning("KinematicFit")
              << "BToKLL - Constrained fit: Skipping candidate due to fit failure: " << e.what();
          continue;
        }
        if (constraint_fitter.success()) {
          auto constraint_p4 = constraint_fitter.fitted_p4();
          constraint_sv_prob = constraint_fitter.prob();
          constraint_pt = constraint_p4.pt();
          constraint_eta = constraint_p4.eta();
          constraint_phi = constraint_p4.phi();
          constraint_mass = constraint_fitter.fitted_candidate().mass();
          constraint_massErr = sqrt(constraint_fitter.fitted_candidate().kinematicParametersError().matrix()(6, 6));
          constraint_mll = (constraint_fitter.daughter_p4(0) + constraint_fitter.daughter_p4(1)).mass();
        }
      }
      cand.addUserFloat("constraint_sv_prob", constraint_sv_prob);
      cand.addUserFloat("constraint_pt", constraint_pt);
      cand.addUserFloat("constraint_eta", constraint_eta);
      cand.addUserFloat("constraint_phi", constraint_phi);
      cand.addUserFloat("constraint_mass", constraint_mass);
      cand.addUserFloat("constraint_massErr", constraint_massErr);
      cand.addUserFloat("constraint_mll", constraint_mll);

      ret_val->push_back(cand);
    }  // for(size_t ll_idx = 0; ll_idx < dileptons->size(); ++ll_idx) {
  }  // for(size_t k_idx = 0; k_idx < kaons->size(); ++k_idx)

  evt.put(std::move(ret_val));
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(BToTrkLLBuilder);
