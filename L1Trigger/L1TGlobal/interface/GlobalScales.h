#ifndef L1TGlobalScales_h
#define L1TGlobalScales_h

/**
 * \class GlobalScales
 *
 *
 * Description: Accessor Class for uGT Result
 *
 * Implementation:
 *    <TODO: enter implementation details>
 *
 */

// system include files
#include <vector>
#include <string>
#include <map>
#include <iosfwd>

// class declaration

namespace l1t {

  class GlobalScales {
  public:
    // constructors
    GlobalScales();

    // destructor
    virtual ~GlobalScales();

  public:
    /// typedef for a single object template
    struct ScaleParameters {
      double etMin;
      double etMax;
      double etStep;
      std::vector<std::pair<double, double>> etBins;

      // Added displaced muons
      double uptMin;
      double uptMax;
      double uptStep;
      std::vector<std::pair<double, double>> uptBins;

      double phiMin;
      double phiMax;
      double phiStep;
      std::vector<std::pair<double, double>> phiBins;

      double etaMin;
      double etaMax;
      double etaStep;
      std::vector<std::pair<double, double>> etaBins;
    };

    inline void setEGScales(ScaleParameters& scales) { m_egScales = scales; }
    inline void setTauScales(ScaleParameters& scales) { m_tauScales = scales; }
    inline void setMuonScales(ScaleParameters& scales) { m_muScales = scales; }

    inline void setJetScales(ScaleParameters& scales) { m_jetScales = scales; }

    inline void setETTScales(ScaleParameters& scales) { m_ettScales = scales; }
    inline void setETTEmScales(ScaleParameters& scales) { m_ettEmScales = scales; }
    inline void setHTTScales(ScaleParameters& scales) { m_httScales = scales; }
    inline void setETMScales(ScaleParameters& scales) { m_etmScales = scales; }
    inline void setETMHfScales(ScaleParameters& scales) { m_etmHfScales = scales; }
    inline void setHTMHfScales(ScaleParameters& scales) { m_htmHfScales = scales; }
    inline void setHTMScales(ScaleParameters& scales) { m_htmScales = scales; }

    virtual void setLUT_CalMuEta(const std::string& lutName, std::vector<long long> lut);
    virtual void setLUT_CalMuPhi(const std::string& lutName, std::vector<long long> lut);
    virtual void setLUT_DeltaEta(const std::string& lutName, std::vector<long long> lut, unsigned int precision);
    virtual void setLUT_DeltaPhi(const std::string& lutName, std::vector<long long> lut, unsigned int precision);
    virtual void setLUT_Pt(const std::string& lutName, std::vector<long long> lut, unsigned int precision);
    virtual void setLUT_Upt(const std::string& lutName,
                            std::vector<long long> lut,
                            unsigned int precision);  // Added for displaced muons
    virtual void setLUT_Cosh(const std::string& lutName, std::vector<long long> lut, unsigned int precision);
    virtual void setLUT_Cos(const std::string& lutName, std::vector<long long> lut, unsigned int precision);
    virtual void setLUT_Sin(const std::string& lutName, std::vector<long long> lut, unsigned int precision);

    inline void setScalesName(const std::string& name) { m_ScaleSetName = name; }
    virtual std::string getScalesName() const;

    inline const ScaleParameters& getMUScales() const { return m_muScales; }
    inline const ScaleParameters& getEGScales() const { return m_egScales; }
    inline const ScaleParameters& getTAUScales() const { return m_tauScales; }
    inline const ScaleParameters& getJETScales() const { return m_jetScales; }
    inline const ScaleParameters& getETTScales() const { return m_ettScales; }
    inline const ScaleParameters& getETTEmScales() const { return m_ettEmScales; }
    inline const ScaleParameters& getETMScales() const { return m_etmScales; }
    inline const ScaleParameters& getETMHFScales() const { return m_etmHfScales; }
    inline const ScaleParameters& getHTMHFScales() const { return m_htmHfScales; }
    inline const ScaleParameters& getHTTScales() const { return m_httScales; }
    inline const ScaleParameters& getHTMScales() const { return m_htmScales; }

    long long getLUT_CalMuEta(const std::string& lutName, int element) const;
    long long getLUT_CalMuPhi(const std::string& lutName, int element) const;
    long long getLUT_DeltaEta(std::string lutName, int element) const;
    long long getLUT_DeltaPhi(std::string lutName, int element) const;
    long long getLUT_Pt(const std::string& lutName, int element) const;
    long long getLUT_Upt(const std::string& lutName, int element) const;  // Added for displaced muons
    long long getLUT_DeltaEta_Cosh(std::string lutName, int element) const;
    long long getLUT_DeltaPhi_Cos(std::string lutName, int element) const;
    long long getLUT_Cos(const std::string& lutName, int element) const;
    long long getLUT_Sin(const std::string& lutName, int element) const;

    unsigned int getPrec_DeltaEta(const std::string& lutName) const;
    unsigned int getPrec_DeltaPhi(const std::string& lutName) const;
    unsigned int getPrec_Pt(const std::string& lutName) const;
    unsigned int getPrec_Upt(const std::string& lutName) const;  // Added for displaced muons
    unsigned int getPrec_DeltaEta_Cosh(const std::string& lutName) const;
    unsigned int getPrec_DeltaPhi_Cos(const std::string& lutName) const;
    unsigned int getPrec_Cos(const std::string& lutName) const;
    unsigned int getPrec_Sin(const std::string& lutName) const;

    virtual void dumpAllLUTs(std::ostream& myCout) const;
    virtual void dumpLUT(std::ostream& myCout, int LUTtype, std::string name) const;
    virtual void print(std::ostream& myCout) const;
    virtual void printScale(ScaleParameters scale, std::ostream& myCout) const;

  private:
  private:
    std::string m_ScaleSetName;

    ScaleParameters m_egScales;
    ScaleParameters m_tauScales;
    ScaleParameters m_muScales;

    ScaleParameters m_jetScales;

    ScaleParameters m_ettScales;
    ScaleParameters m_ettEmScales;
    ScaleParameters m_httScales;
    ScaleParameters m_etmScales;
    ScaleParameters m_etmHfScales;
    ScaleParameters m_htmHfScales;
    ScaleParameters m_htmScales;

    //LUTs
    std::map<std::string, std::vector<long long>> m_lut_CalMuEta;
    std::map<std::string, std::vector<long long>> m_lut_CalMuPhi;
    std::map<std::string, std::vector<long long>> m_lut_DeltaEta;
    std::map<std::string, std::vector<long long>> m_lut_DeltaPhi;
    std::map<std::string, std::vector<long long>> m_lut_Pt;
    std::map<std::string, std::vector<long long>> m_lut_Upt;  // Added for displaced muons
    std::map<std::string, std::vector<long long>> m_lut_Cosh;
    std::map<std::string, std::vector<long long>> m_lut_Cos;
    std::map<std::string, std::vector<long long>> m_lut_Sin;

    //LUTs Precision
    std::map<std::string, unsigned int> m_Prec_DeltaEta;
    std::map<std::string, unsigned int> m_Prec_DeltaPhi;
    std::map<std::string, unsigned int> m_Prec_Pt;
    std::map<std::string, unsigned int> m_Prec_Upt;  // Added for displaced muons
    std::map<std::string, unsigned int> m_Prec_Cosh;
    std::map<std::string, unsigned int> m_Prec_Cos;
    std::map<std::string, unsigned int> m_Prec_Sin;
  };

}  // namespace l1t
#endif
