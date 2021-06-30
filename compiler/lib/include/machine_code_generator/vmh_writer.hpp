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
*******************************************************************************/

#ifndef VMH_WRITER_H_
#define VMH_WRITER_H_

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <stack>

#include "switch_modes.hpp"
#include "encoding_table.hpp"
#include "switch_config.hpp"
#include "analysis-structure.hpp"
#include "number_system_converter.hpp"


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

        void WriteVN_Config(std::shared_ptr<std::vector<std::shared_ptr<MAERI::ReductionNetwork::DBRS_Config>>> DBRS_config,
                            std::shared_ptr<std::vector<std::shared_ptr<MAERI::ReductionNetwork::SGRS_Config>>> SGRS_config,
                            std::shared_ptr<std::map<int, std::pair<int, int>>> DBRS_mapping,
                            std::shared_ptr<std::map<int, std::pair<int, int>>> SGRS_mapping,
                            int num_levels) {
          std::stack<int> stack;
          stack.push(num_levels / 2);
          std::string line = "";

          while (!stack.empty()) {
            int id = stack.pop();
            auto find_SGRS = SGRS_mapping.find(id);
            if (find_SGRS != SGRS_mapping.end()) {
              int level = std::get<0>(SGRS_mapping[id]);
              int pos = std::get<1>(SGRS_mapping[id]);
              line.insert(0, WriteRN_SGRS_One(SGRS_config->single_reduction_switches_[level][pos]));
              stack.push(SGRS_config->single_reduction_switches_[level][pos]->input_ID_L_);
              stack.push(SGRS_config->single_reduction_switches_[level][pos]->input_ID_R_);
            } else {
              auto find_DBRS = DBRS_mapping.find(id);
              if (find_DBRS != DBRS_mapping.end()) {
                int level = std::get<0>(DBRS_mapping[id]);
                int pos = std::get<1>(DBRS_mapping[id]);

                int inorder_id_left = id - 2 * pow(num_levels - 1 - level, 2);
                auto find_DBRS_left = DBRS_mapping.find(inorder_id_left);
                if (find_DBRS_left != DBRS_mapping.end()) {
                  int pos_left = std::get<1>(DBRS_mapping[inorder_id_left]);
                  if (pos == pos_left) {
                    line.insert(0, WriteRN_DBRS_Right(DBRS_config->double_reduction_switches_[level][pos]));
                    stack.push(DBRS_config->double_reduction_switches_[level][pos]->input_ID_RL_);
                    stack.push(DBRS_config->double_reduction_switches_[level][pos]->input_ID_RR_);
                  } else {
                    line.insert(0, WriteRN_DBRS_Left(DBRS_config->double_reduction_switches_[level][pos])); 
                    stack.push(DBRS_config->double_reduction_switches_[level][pos]->input_ID_LL_);
                    stack.push(DBRS_config->double_reduction_switches_[level][pos]->input_ID_LR_);
                  }
                }
              }
            }
          }

          while (line.size() >= 32) {
            std::string flush_str = bin2hex.GetHexString(line.substr(0, 32));
            outputFile_ << flush_str + "\n";
            line = line.substr(32);
          }

        }

        /*void WriteVN_Config(std::shared_ptr<std::vector<std::shared_ptr<MAERI::ReductionNetwork::DBRS_Config>>> DBRS_config,
                            std::shared_ptr<std::vector<std::shared_ptr<MAERI::ReductionNetwork::SGRS_Config>>> SGRS_config,
                            std::shared_ptr<std::map<int, std::pair<int, int>>> DBRS_mapping,
                            std::shared_ptr<std::map<int, std::pair<int, int>>> SGRS_mapping,
                            int num_levels) {
          int count;
          std::string line = "";
          for (int i = 0; i < DBRS_mapping.size() + SGRS_mapping.size(); i++) {
            auto find_SGRS = SGRS_mapping.find(i);
            if (find_SGRS != SGRS_mapping.end()) {
              int level = std::get<0>(SGRS_mapping[i]);
              int pos = std::get<1>(SGRS_mapping[i]);
              line.insert(0, WriteRN_SGRS_One(SGRS_config->single_reduction_switches_[level][pos]));
            } else {
              auto find_DBRS = DBRS_mapping.find(i);
              if (find_DBRS != DBRS_mapping.end()) {
                int level = std::get<0>(DBRS_mapping[i]);
                int pos = std::get<1>(DBRS_mapping[i]);

                int inorder_id_left = i - 2 * pow(num_levels - 1 - level, 2);
                auto find_DBRS_left = DBRS_mapping.find(inorder_id_left);
                if (find_DBRS_left != DBRS_mapping.end()) {
                  int pos_left = std::get<1>(DBRS_mapping[inorder_id_left]);
                  if (pos == pos_left) {
                    line.insert(0, WriteRN_DBRS_Right(DBRS_config->double_reduction_switches_[level][pos])); 
                  } else {
                    line.insert(0, WriteRN_DBRS_Left(DBRS_config->double_reduction_switches_[level][pos])); 
                  }
                }
              }
            }
          }

          std::string preorder_line = "";


          while (line.size() >= 32) {
            std::string flush_str = bin2hex.GetHexString(line.substr(0, 32));
            outputFile_ << flush_str + "\n";
            line = line.substr(32);
          }

          if(count == 7) {
                //Flush
            std::string flush_str = bin2hex.GetHexString(line);
            outputFile_ << flush_str + "\n";
            line = "";
            count = 0;
          } else {
            count++;
          }

        }*/

        std::string WriteRN_SGRS_One(std::shared_ptr<<MAERI::ReductionNetwork::SGRS_Config>> it) {
          std::string line = "";
          auto mode = it->mode_;
          auto genOutput = it->genOutput_;
          if(genOutput) {
            line.insert(0, "1");
          }
          else {
            line.insert(0, "0");
          }
          switch(mode) {
            case ReductionNetwork::SGRS_Mode::Idle:
              line.insert(0, ISA::SGRS_MODE_IDLE);
              break;
            case ReductionNetwork::SGRS_Mode::AddTwo:
              line.insert(0, ISA::SGRS_MODE_ADDTWO);
              break;
            case ReductionNetwork::SGRS_Mode::FlowLeft:
              line.insert(0, ISA::SGRS_MODE_FLOWLEFT);
              break;
            case ReductionNetwork::SGRS_Mode::FlowRight:
              line.insert(0, ISA::SGRS_MODE_FLOWRIGHT);
              break;
            default:
              line.insert(0, ISA::SGRS_MODE_IDLE);
              break;
          }
          return line;
        }

        std:string WriteRN_DBRS_Right(std::shared_ptr<<MAERI::ReductionNetwork::SGRS_Config>> it) {
          std::string line = "";
          auto modeR = it->modeR_;
          auto genOutputR = it->genOutputR_;
          if(genOutputR) {
            line.insert(0, "1");
          }
          else {
            line.insert(0, "0");
          }
          switch(modeR) {
            case ReductionNetwork::DBRS_SubMode::Idle:
              line.insert(0, ISA::DBRS_MODE_IDLE);
              break;
            case ReductionNetwork::DBRS_SubMode::AddOne:
              line.insert(0, ISA::DBRS_MODE_ADDONE);
              break;
            case ReductionNetwork::DBRS_SubMode::AddTwo:
              line.insert(0, ISA::DBRS_MODE_ADDTWO);
              break;
            case ReductionNetwork::DBRS_SubMode::AddThree:
              line.insert(0, ISA::DBRS_MODE_ADDTHREE);
              break;
            default:
              line.insert(0, ISA::DBRS_MODE_PADDING);
              break;
          }
          return line;
        }

        std:string WriteRN_DBRS_Left(std::shared_ptr<<MAERI::ReductionNetwork::SGRS_Config>> it) {
           std::string line = "";
          auto modeL = it->modeL_;
          auto genOutputL = it->genOutputL_;
          if(genOutputL) {
            line.insert(0, "1");
          }
          else {
            line.insert(0, "0");
          }
          switch(modeL) {
            case ReductionNetwork::DBRS_SubMode::Idle:
              line.insert(0, ISA::DBRS_MODE_IDLE);
              break;
            case ReductionNetwork::DBRS_SubMode::AddOne:
              line.insert(0, ISA::DBRS_MODE_ADDONE);
              break;
            case ReductionNetwork::DBRS_SubMode::AddTwo:
              line.insert(0, ISA::DBRS_MODE_ADDTWO);
              break;
            case ReductionNetwork::DBRS_SubMode::AddThree:
              line.insert(0, ISA::DBRS_MODE_ADDTHREE);
              break;
            default:
              line.insert(0, ISA::DBRS_MODE_PADDING);
              break;
          }
          return line;
        }

        void WriteRN_DBRS_Config(std::shared_ptr<std::vector<std::shared_ptr<MAERI::ReductionNetwork::DBRS_Config>>> config) {
          std::string line = "";
          int count = 0;
          for(auto it: *config) {
            auto modeL = it->modeL_;
            auto modeR = it->modeR_;
            auto genOutputL = it->genOutputL_;
            auto genOutputR = it->genOutputR_;

            line.insert(0, ISA::DBRS_MODE_PADDING);

            if(genOutputR) {
              line.insert(0, "1");
            }
            else {
              line.insert(0, "0");
            }

            if(genOutputL) {
              line.insert(0, "1");
            }
            else {
              line.insert(0, "0");
            }


            switch(modeR) {
              case ReductionNetwork::DBRS_SubMode::Idle:
                line.insert(0, ISA::DBRS_MODE_IDLE);
                break;
              case ReductionNetwork::DBRS_SubMode::AddOne:
                line.insert(0, ISA::DBRS_MODE_ADDONE);
                break;
              case ReductionNetwork::DBRS_SubMode::AddTwo:
                line.insert(0, ISA::DBRS_MODE_ADDTWO);
                break;
              case ReductionNetwork::DBRS_SubMode::AddThree:
                line.insert(0, ISA::DBRS_MODE_ADDTHREE);
                break;
              default:
                line.insert(0, ISA::DBRS_MODE_PADDING);
                break;
            }

            switch(modeL) {
              case ReductionNetwork::DBRS_SubMode::Idle:
                line.insert(0, ISA::DBRS_MODE_IDLE);
                break;
              case ReductionNetwork::DBRS_SubMode::AddOne:
                line.insert(0, ISA::DBRS_MODE_ADDONE);
                break;
              case ReductionNetwork::DBRS_SubMode::AddTwo:
                line.insert(0, ISA::DBRS_MODE_ADDTWO);
                break;
              case ReductionNetwork::DBRS_SubMode::AddThree:
                line.insert(0, ISA::DBRS_MODE_ADDTHREE);
                break;
              default:
                line.insert(0, ISA::DBRS_MODE_PADDING);
                break;
            }


            if(count == 3) {
              //Flush
              std::string flush_str = bin2hex.GetHexString(line);
              outputFile_ << flush_str + "\n";
              line = "";
              count = 0;
            }
            else {
              count++;
            }
          } // End of for(auto it: *config)

          if(line != "") {
            int remaining = 32 - line.length();
            for(int pad = 0; pad < remaining; pad ++) {
              line.insert(0, "0");
            }

            std::string flush_str = bin2hex.GetHexString(line);
            outputFile_ << flush_str + "\n";
          }

        } // End of void WriteRN_DBRS_Config

        void WriteRN_SGRS_Config(std::shared_ptr<std::vector<std::shared_ptr<MAERI::ReductionNetwork::SGRS_Config>>> config) {
          std::string line = "";

          int count = 0;
          for(auto it: *config) {
            auto mode = it->mode_;
            auto genOutput = it->genOutput_;

            line.insert(0, ISA::SGRS_MODE_PADDING);

            if(genOutput) {
              line.insert(0, "1");
            }
            else {
              line.insert(0, "0");
            }

            switch(mode) {
              case ReductionNetwork::SGRS_Mode::Idle:
                line.insert(0, ISA::SGRS_MODE_IDLE);
                break;
              case ReductionNetwork::SGRS_Mode::AddTwo:
                line.insert(0, ISA::SGRS_MODE_ADDTWO);
                break;
              case ReductionNetwork::SGRS_Mode::FlowLeft:
                line.insert(0, ISA::SGRS_MODE_FLOWLEFT);
                break;
              case ReductionNetwork::SGRS_Mode::FlowRight:
                line.insert(0, ISA::SGRS_MODE_FLOWRIGHT);
                break;
              default:
                line.insert(0, ISA::SGRS_MODE_IDLE);
                break;
            }

            if(count == 7) {
              //Flush
              std::string flush_str = bin2hex.GetHexString(line);
              outputFile_ << flush_str + "\n";
              line = "";
              count = 0;
            }
            else {
              count++;
            }
          } // End of for(auto it: *config)


          if(line != "") {
            int remaining = 32 - line.length();
            for(int pad = 0; pad < remaining; pad ++) {
              line.insert(0, "0");
            }

            std::string flush_str = bin2hex.GetHexString(line);
            outputFile_ << flush_str + "\n";
          }

        }// End of void WriteRN_SGRS_Config
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
