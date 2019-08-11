//-------------------------------------------------------------------------
//   Copyright 2002-2019 National Technology & Engineering Solutions of
//   Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
//   NTESS, the U.S. Government retains certain rights in this software.
//
//   This file is part of the Xyce(TM) Parallel Electrical Simulator.
//
//   Xyce(TM) is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Xyce(TM) is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Xyce(TM).
//   If not, see <http://www.gnu.org/licenses/>.
//-------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Purpose       : This file is a class to manage measure statements in a sim.
//
// Special Notes :
//
// Creator       : Richard Schiek, SNL, Electrical and Microsystem Modeling
//
// Creation Date : 03/10/2009
//
//
//
//
//-----------------------------------------------------------------------------

#ifndef  Xyce_N_IO_MeasureManager_H
#define Xyce_N_IO_MeasureManager_H

#include <list>
#include <string>
#include <iostream>

#include <N_IO_fwd.h>
#include <N_PDS_fwd.h>
#include <N_UTL_fwd.h>
#include <N_IO_Measure_fwd.h>

#include <N_ANP_AnalysisManager.h>
#include <N_ANP_RegisterAnalysis.h>
#include <N_ANP_StepEvent.h>
#include <N_ANP_SweepParam.h>
#include <N_LAS_Vector.h>
#include <N_UTL_Listener.h>
#include <N_UTL_NodeSymbols.h>
#include <N_UTL_Op.h>

namespace Xyce {
namespace IO {
namespace Measure {

//-----------------------------------------------------------------------------
// Class         : MeasureManager
// Purpose       : This is a manager class for handling measure statements
//                 in a simulation
// Special Notes : 
// Creator       : Richard Schiek, Electrical and Microsystems Modeling
// Creation Date : 3/10/2009
//-----------------------------------------------------------------------------
class Manager : public Util::Listener<Analysis::StepEvent>
{
  typedef std::vector<Base *> MeasurementVector;

public:
  Manager(const std::string &netlist_filename);
  ~Manager();

  void notify(const Analysis::StepEvent &step_event);

  // register options from .OPTIONS MEASURE lines
  bool registerMeasureOptions(const Util::OptionBlock & option_block);

  // these flags (for output to .mt files and stdout) are set based on the
  // .OPTIONS MEASURE MEASPRINT value
  bool isMeasGlobalPrintEnabled() const { return enableMeasGlobalPrint_; }
  bool isMeasGlobalVerbosePrintEnabled() const { return enableMeasGlobalVerbosePrint_; }

  // other getters for other .OPTIONS MEASURE variables and flags
  int getMeasDgt() const { return measDgt_; }
  bool isMeasDgtGiven() const { return measDgtGiven_; }
  bool getMeasFail() const { return measFail_; }
  bool isMeasFailGiven() const { return measFailGiven_; }
  double getMeasGlobalDefaultVal() const { return measGlobalDefaultVal_; }
  bool isMeasGlobalDefaultValGiven() const { return measGlobalDefaultValGiven_; }

  // Return true if .measure analysis is being performed on any variables.
  bool isMeasureActive() const { return (!allMeasuresList_.empty()); }

  // add .measure line from netlist to list of things to measure.
  bool addMeasure(const Manager &measureMgr, const Util::OptionBlock & measureLine);

  void makeMeasureOps(Parallel::Machine comm, const Util::Op::BuilderManager &op_builder_manager);

  // used to check agreement between analysis type and measure mode
  bool checkMeasureModes(const Analysis::Mode analysisMode);

  // Called during the simulation to update the measure objects held by this class
  // To keep things obvious, use separate calls for TRAN, DC and AC
  void updateTranMeasures(
    Parallel::Machine comm,
    const double circuitTime,
    const Linear::Vector *solnVec,
    const Linear::Vector *stateVec,
    const Linear::Vector *storeVec,
    const Linear::Vector *lead_current_vector,
    const Linear::Vector *junction_voltage_vector,
    const Linear::Vector *lead_current_dqdt_vector);

  void updateDCMeasures(
    Parallel::Machine comm,
    const std::vector<Analysis::SweepParam> & dcParamsVec,
    const Linear::Vector *solnVec,
    const Linear::Vector *stateVec,
    const Linear::Vector *storeVec,
    const Linear::Vector *lead_current_vector,
    const Linear::Vector *junction_voltage_vector,
    const Linear::Vector *lead_current_dqdt_vector);

  void updateACMeasures(
    Parallel::Machine comm,
    const double frequency,
    const Linear::Vector *solnVec,
    const Linear::Vector *imaginaryVec,
    const Util::Op::RFparamsData *RFparams);

  void outputResultsToMTFile(int stepNumber) const;
  std::ostream & outputResults(std::ostream& outputStream) const;
  std::ostream & outputVerboseResults(std::ostream& outputStream, double endSimTime=0) const;

  const Base *find(const std::string &name) const;

  void remeasure(
    N_PDS_Comm &pds_comm,
    const std::string &netlist_filename,
    const std::string &remeasure_path,
    const char& analysisName,
    Util::Op::BuilderManager &op_builder_manager,
    OutputMgr &output_manager,
    Analysis::AnalysisManager &analysis_manager,
    Analysis::AnalysisCreatorRegistry &analysis_registry,
    Util::SymbolTable &symbol_table);

  bool getMeasureValue(const std::string &name, double &value) const;

  // used to set/get the file suffix (mt, ma or ms) used for both measure and remeasure output.
  void setMeasureOutputFileSuffix(const Analysis::Mode analysisMode);  
  std::string getMeasureOutputFileSuffix() const { return measureOutputFileSuffix_;} 

  //added to help register lead currents with device manager
  std::set<std::string> getDevicesNeedingLeadCurrents() { return devicesNeedingLeadCurrents_; }

private:
  std::string           netlistFilename_;
  std::string           measureOutputFileSuffix_;

  // Controls where the measure output appears, as a global option for all .MEASURE statements.  
  // Both .mt0 file and stdout, only stdout, or neither.
  bool enableMeasGlobalPrint_;
  bool enableMeasGlobalVerbosePrint_;
  // used for .OPTIONS MEASURE MEASDGT
  int measDgt_; 
  bool measDgtGiven_;
  // used for .OPTIONS MEASURE MEASFAIL
  bool measFail_;
  bool measFailGiven_;
  // used for .OPTIONS MEASURE MEASOUT
  bool measOut_;
  bool measOutGiven_;
  // used for .OPTIONS MEASURE DEFAULT_VAL
  double measGlobalDefaultVal_;
  bool measGlobalDefaultValGiven_;

  MeasurementVector     allMeasuresList_;
  MeasurementVector     activeMeasuresList_;
  
  //added to help register lead currents with device manager
  std::set<std::string> devicesNeedingLeadCurrents_;   
};

bool registerPkgOptionsMgr(Manager &manager, PkgOptionsMgr &options_manager);

} // namespace Measure
} // namespace IO
} // namespace Xyce

#endif  // Xyce_N_IO_MeasureManager_H
