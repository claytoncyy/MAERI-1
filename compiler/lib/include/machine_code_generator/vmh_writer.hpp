/******************************************************************************
Copyright (c) 2019 Georgia Instititue of Technology
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Author : Hyoukjun Kwon (hyoukjun@gatech.edu)
Update (July 2021): Yangyu Chen (yangyuchen@gatech.edu)

*******************************************************************************/

#ifndef VMH_WRITER_H_
#define VMH_WRITER_H_

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <stack>
#include <utility>

#include "switch_modes.hpp"
#include "encoding_table.hpp"
#include "switch_config.hpp"
#include "analysis-structure.hpp"
#include "number_system_converter.hpp"

//#define DEBUG

namespace MAERI {
  namespace MachineCodeGenerator {

    class VmhWriter {
      protected:
        std::string filename_;
        std::ofstream outputFile_;

      public:
        VmhWriter(std::string filename) :
          filename_(filename) {
          outputFile_.open(filename);
        }
    }; // End of class VmhWriter

    class RNConfigWriter : public VmhWriter {
      protected:
        BinaryToHex bin2hex;

      public:
        RNConfigWriter(std::string filename) :
          VmhWriter(filename) {
          outputFile_ << "@000\n";
        }

        void WriteVN_Config(std::vector<std::vector<std::shared_ptr<MAERI::ReductionNetwork::DoubleReductionSwitch>>> double_reduction_switches_,
                            std::vector<std::vector<std::shared_ptr<MAERI::ReductionNetwork::SingleReductionSwitch>>> single_reduction_switches_,
                            std::map<int, std::pair<int, int>> DBRS_mapping,
                            std::map<int, std::pair<int, int>> SGRS_mapping,
                            int num_levels,
                            int num_adder_switches) {
          std::stack<int> stack;
          stack.push(num_adder_switches / 2);
          std::string line = "";
          int count = 0;
#ifdef DEBUG
          std::cout << "numLvs: " << num_levels << ", num_adder_switches: " << num_adder_switches << std::endl;
#endif
          while (!stack.empty()) {
            int id = stack.top();
            stack.pop();
            if (id == -1) {
              continue;
            }
#ifdef DEBUG
            std::cout << "id:" << id << std::endl;
#endif
            std::map<int, std::pair<int, int>>::iterator it;
            it = SGRS_mapping.find(id);
            if (it != SGRS_mapping.end()) {
              int level = std::get<0>(SGRS_mapping[id]);
              int pos = std::get<1>(SGRS_mapping[id]);
#ifdef DEBUG
              std::cout << "Level & position:" << level << ", " << pos << std::endl;
#endif
              line.append(WriteRN_SGRS_One(single_reduction_switches_[level][pos]));
#ifdef DEBUG
              std::cout << "ID_L: " << single_reduction_switches_[level][pos]->GetInput_ID_L() << std::endl;
              std::cout << "ID_R: " << single_reduction_switches_[level][pos]->GetInput_ID_R() << std::endl;
#endif
              stack.push(single_reduction_switches_[level][pos]->GetInput_ID_L());
              stack.push(single_reduction_switches_[level][pos]->GetInput_ID_R());
            } else {
              it = DBRS_mapping.find(id);
              if (it != DBRS_mapping.end()) {
                int level = std::get<0>(DBRS_mapping[id]);
                int pos = std::get<1>(DBRS_mapping[id]);  
#ifdef DEBUG
                std::cout << "DBRS Level & position:" << level << ", " << pos << std::endl;
#endif
                int inorder_id_left = id - 2 * pow(2, num_levels - 1 - level);
                it = DBRS_mapping.find(inorder_id_left);
                std::map<int, std::pair<int, int>>::iterator it_left = SGRS_mapping.find(inorder_id_left);
#ifdef DEBUG
                std::cout << "inorder_id_left: " << inorder_id_left << std::endl;
#endif
                if (it != DBRS_mapping.end()) {
                  int pos_left = std::get<1>(DBRS_mapping[inorder_id_left]);
                  if (pos == pos_left) {
                    line.append(WriteRN_DBRS_Right(double_reduction_switches_[level][pos]));
#ifdef DEBUG
                    std::cout << "ID_RL: " << double_reduction_switches_[level][pos]->GetInput_ID_RL() << std::endl;
                    std::cout << "ID_RR: " << double_reduction_switches_[level][pos]->GetInput_ID_RR() << std::endl;
#endif
                    stack.push(double_reduction_switches_[level][pos]->GetInput_ID_RL());
                    stack.push(double_reduction_switches_[level][pos]->GetInput_ID_RR());
                  } else {
                    line.append(WriteRN_DBRS_Left(double_reduction_switches_[level][pos]));
#ifdef DEBUG 
                    std::cout << "ID_LL: " << double_reduction_switches_[level][pos]->GetInput_ID_LL() << std::endl;
                    std::cout << "ID_LR: " << double_reduction_switches_[level][pos]->GetInput_ID_LR() << std::endl;
#endif
                    stack.push(double_reduction_switches_[level][pos]->GetInput_ID_LL());
                    stack.push(double_reduction_switches_[level][pos]->GetInput_ID_LR());
                  }
                } else if (it_left != SGRS_mapping.end()) {
                  line.append(WriteRN_DBRS_Left(double_reduction_switches_[level][pos]));
#ifdef DEBUG
                  std::cout << "ID_LL: " << double_reduction_switches_[level][pos]->GetInput_ID_LL() << std::endl;
                  std::cout << "ID_LR: " << double_reduction_switches_[level][pos]->GetInput_ID_LR() << std::endl;
#endif
                  stack.push(double_reduction_switches_[level][pos]->GetInput_ID_LL());
                  stack.push(double_reduction_switches_[level][pos]->GetInput_ID_LR());
                }
              }
            }
            if(count == 7) {
              //Flush
              outputFile_ << line + "\n";
              line = "";
              count = 0;
            } else {
              count++;
            }
          }

          if(line != "") {
            outputFile_ << line + "\n";
          }

        }

        std::string WriteRN_SGRS_One(std::shared_ptr<MAERI::ReductionNetwork::SingleReductionSwitch> it) {
          std::string line = "";
          auto mode = it->GetMode();
          auto genOutput = it->GetGenOutput();
          if(genOutput) {
            line.append("1");
          }
          else {
            line.append("0");
          }
          switch(mode) {
            case ReductionNetwork::SGRS_Mode::Idle:
              line.append(ISA::SGRS_MODE_IDLE);
              break;
            case ReductionNetwork::SGRS_Mode::AddTwo:
              line.append(ISA::SGRS_MODE_ADDTWO);
              break;
            case ReductionNetwork::SGRS_Mode::FlowLeft:
              line.append(ISA::SGRS_MODE_FLOWLEFT);
              break;
            case ReductionNetwork::SGRS_Mode::FlowRight:
              line.append(ISA::SGRS_MODE_FLOWRIGHT);
              break;
            default:
              line.append(ISA::SGRS_MODE_IDLE);
              break;
          }
#ifdef DEBUG
          std::cout << "SGRS Command:" << line << std::endl;
#endif
          return line;
        }

        std::string WriteRN_DBRS_Right(std::shared_ptr<MAERI::ReductionNetwork::DoubleReductionSwitch> it) {
          std::string line = "";
          auto modeR = it->GetModeR();
          auto genOutputR = it->GetGenOutputR();
          if(genOutputR) {
            line.append("1");
          }
          else {
            line.append("0");
          }
          switch(modeR) {
            case ReductionNetwork::DBRS_SubMode::Idle:
              line.append(ISA::DBRS_MODE_IDLE);
              break;
            case ReductionNetwork::DBRS_SubMode::AddOne:
              line.append(ISA::DBRS_MODE_ADDONE);
              break;
            case ReductionNetwork::DBRS_SubMode::AddTwo:
              line.append(ISA::DBRS_MODE_ADDTWO);
              break;
            case ReductionNetwork::DBRS_SubMode::AddThree:
              line.append(ISA::DBRS_MODE_ADDTHREE);
              break;
            default:
              line.append(ISA::DBRS_MODE_IDLE);
              break;
          }
#ifdef DEBUG
          std::cout << "DBRS_R Command:" << line << std::endl;
#endif
          return line;
        }

        std::string WriteRN_DBRS_Left(std::shared_ptr<MAERI::ReductionNetwork::DoubleReductionSwitch> it) {
          std::string line = "";
          auto modeL = it->GetModeL();
          auto genOutputL = it->GetGenOutputL();
          if(genOutputL) {
            line.append("1");
          }
          else {
            line.append("0");
          }
          switch(modeL) {
            case ReductionNetwork::DBRS_SubMode::Idle:
              line.append(ISA::DBRS_MODE_IDLE);
              break;
            case ReductionNetwork::DBRS_SubMode::AddOne:
              line.append(ISA::DBRS_MODE_ADDONE);
              break;
            case ReductionNetwork::DBRS_SubMode::AddTwo:
              line.append(ISA::DBRS_MODE_ADDTWO);
              break;
            case ReductionNetwork::DBRS_SubMode::AddThree:
              line.append(ISA::DBRS_MODE_ADDTHREE);
              break;
            default:
              line.append(ISA::DBRS_MODE_PADDING);
              break;
          }
#ifdef DEBUG
          std::cout << "DBRS_L Command:" << line << std::endl;
#endif
          return line;
        }
    }; // End of class RNConfigWriter

    class TileInfoWriter : public VmhWriter {
      protected:
        IntToHex int2hex;
      public:
        TileInfoWriter(std::string filename) :
          VmhWriter(filename) {
          outputFile_ << "@00\n";
        }

        void WriteTileInfo(std::shared_ptr<maestro::LoopInfoTable> loopInfoTable, int numMultSwitches, int vnSz, int numMappedVNs) {
          std::string line = "";
          auto loopK = loopInfoTable->FindLoops("K")->front();
          auto loopC = loopInfoTable->FindLoops("C")->front();
          auto loopR = loopInfoTable->FindLoops("R")->front();
          auto loopS = loopInfoTable->FindLoops("S")->front();
          auto loopY = loopInfoTable->FindLoops("Y")->front();
          auto loopX = loopInfoTable->FindLoops("X")->front();

          line += int2hex.GetHexString(loopK->GetBound(), 4);
          line += int2hex.GetHexString(loopK->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString(loopK->GetBound() % loopK->GetTileSz(), 4);
          line += int2hex.GetHexString(loopK->GetBound() / loopK->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";


          line += int2hex.GetHexString(loopC->GetBound(), 4);
          line += int2hex.GetHexString(loopC->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString(loopC->GetBound() % loopC->GetTileSz(), 4);
          line += int2hex.GetHexString(loopC->GetBound() / loopC->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";


          line += int2hex.GetHexString(loopR->GetBound(), 4);
          line += int2hex.GetHexString(loopR->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString(loopR->GetBound() % loopR->GetTileSz(), 4);
          line += int2hex.GetHexString(loopR->GetBound() / loopR->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";


          line += int2hex.GetHexString(loopS->GetBound(), 4);
          line += int2hex.GetHexString(loopS->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString(loopS->GetBound() % loopS->GetTileSz(), 4);
          line += int2hex.GetHexString(loopS->GetBound() / loopS->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString(loopY->GetBound(), 4);
          line += int2hex.GetHexString(loopY->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString((loopY->GetBound() - loopR->GetBound() +1 ) % loopY->GetTileSz(), 4);
          line += int2hex.GetHexString((loopY->GetBound() - loopR->GetBound() +1 ) / loopY->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";


          line += int2hex.GetHexString(loopX->GetBound(), 4);
          line += int2hex.GetHexString(loopX->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString((loopX->GetBound() - loopS->GetBound() +1 ) % loopX->GetTileSz(), 4);
          line += int2hex.GetHexString((loopX->GetBound() - loopS->GetBound() +1 ) / loopX->GetTileSz(), 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString(numMultSwitches, 4);
          line += int2hex.GetHexString(numMappedVNs, 4);
          outputFile_ << line << "\n";
          line = "";

          line += int2hex.GetHexString(vnSz, 8);
          outputFile_ << line << "\n";
          line = "";

        }

    }; // End of class TileInfoWriter

  }; // End of namesapce MachineCodeGenerator
}; // End of namespace MAERI

#endif
