/******************************************************************************
Copyright (c) 2019 Georgia Instititue of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Author: Hyoukjun Kwon (hyoukjun@gatech.edu)

*******************************************************************************/

#ifndef RN_ABSTRACT_REDUCTION_NETWORK_H_
#define RN_ABSTRACT_REDUCTION_NETWORK_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cmath>
#include <cassert>
#include <map>

#include "switch_modes.hpp"
#include "switch_config.hpp"
#include "single_reduction_switch.hpp"
#include "double_reduction_switch.hpp"

//#define DEBUG


namespace MAERI {
  namespace ReductionNetwork {
    class AbstractReductionNetwork {
      protected:
        int num_mult_switches_;
        int num_levels_;
        int vn_size_;
        int vn_num_;

        bool non_uniform;

        std::map <int, std::pair<int, int>> inorder_single_reduction_swtiches;
        std::map <int, std::pair<int, int>> inorder_double_reduction_swtiches;

        std::vector<std::vector<std::shared_ptr<SingleReductionSwitch>>> single_reduction_switches_;
        std::vector<std::vector<std::shared_ptr<DoubleReductionSwitch>>> double_reduction_switches_;

      private:
        void IdleSwitchesProcess(int start_index) {
          int num_adder_switches = num_mult_switches_ - 1;
          for (int inPrt = start_index; inPrt < num_mult_switches_; inPrt++) {
              if(inPrt <2) {
                single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, inPrt % 2);
                inorder_single_reduction_swtiches.insert(std::make_pair(0, std::make_pair(num_levels_ - 1, 0)));
              }
              else if(inPrt > num_mult_switches_-3) {
                single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, inPrt % 2);
                inorder_single_reduction_swtiches.insert(std::make_pair(num_adder_switches - 1, std::make_pair(num_levels_ - 1, 1)));
              }
              else {
                int dbrs_id = (inPrt - 2)/4;
                int port_id = (inPrt -2) % 4;
                int inorder_id =  2 * (inPrt / 2);
                //std::cout << "inorder_id: " << inorder_id << ", dbrs_id: " << dbrs_id << ", port_id: " << port_id << std::endl;
                double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id);
                inorder_double_reduction_swtiches.insert(std::make_pair(inorder_id, std::make_pair(num_levels_ - 1, dbrs_id)));
              }
            }
        }

        void LowestLevelSingleVNCase() {
          int max_vn_num = num_mult_switches_ / 2;
          int num_adder_switches = num_mult_switches_ - 1;
          if (vn_num_ > max_vn_num) {
            std::cout << "ERROR: Number of VNs exceeds the maximum number(num_multiplier / 2) allowed for VN SIZE 1" << std::endl;
            return;
          } else {
            for (int vn_id = 0; vn_id < max_vn_num; vn_id++) {
              if (vn_id < vn_num_) {
                auto compile_packet = std::make_shared<CompilePacket>(vn_id, vn_size_, 1);
                if (vn_id == 0) {
                  single_reduction_switches_[num_levels_-1][0]->PutPacket(compile_packet, 0);
                  //single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, 0);
                  //single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, 1);
                  inorder_single_reduction_swtiches.insert(std::make_pair(0, std::make_pair(num_levels_ - 1, 0)));
                } else if (vn_id == max_vn_num - 1) {
                  single_reduction_switches_[num_levels_-1][1]->PutPacket(compile_packet, 0);
                  //single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, 0);
                  //single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, 1);
                  inorder_single_reduction_swtiches.insert(std::make_pair(num_adder_switches - 1, std::make_pair(num_levels_ - 1, 1)));
                } else {
                  int dbrs_id = (vn_id - 1) / 2;
                  int pos_id = (vn_id - 1) % 2;
                  int port_id = pos_id == 0 ? 0 : 3;
                  int inorder_id = vn_id * 2;
                  double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id);
                  //double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id);
                  //double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id + 1);
                  inorder_double_reduction_swtiches.insert(std::make_pair(inorder_id, std::make_pair(num_levels_ - 1, dbrs_id)));
                }
              } else {
                IdleSwitchesProcess(vn_num_ * 2);
              }
            }
          }
        }

        void LowestLevelNormalCase() {
          int num_adder_switches = num_mult_switches_ - 1;
          for(int inPrt = 0; inPrt < num_mult_switches_; inPrt++) {
            int vn_id = inPrt / vn_size_;
            //int num_vns = num_mult_switches_ / vn_size_;
            auto compile_packet = std::make_shared<CompilePacket>(vn_id, vn_size_, 1);
            //            for (int sw = 0; sw < num_mult_switches_; sw++) {
            if(vn_id < vn_num_) {
              if(inPrt <2) {
#ifdef DEBUG
                std::cout << "SGRS[" << num_levels_-1 << "][0] receives an initial packet to port " << inPrt %2 << std::endl;
#endif
                single_reduction_switches_[num_levels_-1][0]->PutPacket(compile_packet, inPrt %2 );
                single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, inPrt % 2);
                inorder_single_reduction_swtiches.insert(std::make_pair(0, std::make_pair(num_levels_ - 1, 0)));
              }
              else if(inPrt > num_mult_switches_-3) {
#ifdef DEBUG
                std::cout << "SGRS[" << num_levels_-1 << "][1] receives an initial packet to port " << inPrt %2 << std::endl;
#endif
                single_reduction_switches_[num_levels_-1][1]->PutPacket(compile_packet, inPrt %2 );
                single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, inPrt % 2);
                inorder_single_reduction_swtiches.insert(std::make_pair(num_adder_switches - 1, std::make_pair(num_levels_ - 1, 1)));
              }
              else {
                int dbrs_id = (inPrt - 2)/4;
                int port_id = (inPrt -2) % 4;
#ifdef DEBUG
                std::cout << "DBRS[" << num_levels_-1 << "]["<< dbrs_id << "] receives an initial packet to port " << port_id << std::endl;
#endif
                int inorder_id =  2 * (inPrt / 2);
                double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id);
                double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id);
                std::cout << "inorder_id: " << inorder_id << ", dbrs_id: " << dbrs_id << ", port_id: " << port_id << std::endl;
                inorder_double_reduction_swtiches.insert(std::make_pair(inorder_id, std::make_pair(num_levels_ - 1, dbrs_id)));
              }
            } else {
              IdleSwitchesProcess(inPrt);
              break;
            }
          }
#ifdef DEBUG
          std::cout << std::endl;
#endif
        }

      public:
        AbstractReductionNetwork(int numMultSwitches, int vn_size, int vn_num, bool non_uniform) :
          num_mult_switches_(numMultSwitches),
          vn_size_(vn_size),
          vn_num_(vn_num),
          non_uniform(non_uniform)  {

          int numLvs = static_cast<int>(log2(numMultSwitches));

          num_levels_ = numLvs;

          for(int lv = 0; lv < numLvs ; lv++) {
            std::vector<std::shared_ptr<SingleReductionSwitch>> lvSgrs;
            single_reduction_switches_.push_back(lvSgrs);

            std::vector<std::shared_ptr<DoubleReductionSwitch>> lvdbrs;
            double_reduction_switches_.push_back(lvdbrs);
          }

          int sgrs_swid = 0;
          int dbrs_swid = 0;
          for(int lv = 0; lv < numLvs ; lv++) {
            int num_sgrs_in_lv = (lv == 0)? 1 : 2;
            for(int sw = 0; sw < num_sgrs_in_lv; sw++) {
              auto new_sgrs = std::make_shared<SingleReductionSwitch>();
              new_sgrs->switch_id = sgrs_swid;
              single_reduction_switches_[lv].push_back(new_sgrs);
#ifdef DEBUG
              std::cout << "SGRS[" << lv << "][" << sw << "] is being initialized" << std::endl;
#endif
              sgrs_swid++;
            }

            int num_dbrs_in_lv = GetNumDBRS(lv);
            if(num_dbrs_in_lv > 0) {
              for(int sw = 0; sw < num_dbrs_in_lv; sw++) {
                auto new_dbrs = std::make_shared<DoubleReductionSwitch>();
                new_dbrs->switch_id = dbrs_swid;
                double_reduction_switches_[lv].push_back(new_dbrs);
#ifdef DEBUG
                std::cout << "DBRS[" << lv << "][" << sw << "] is being initialized" << std::endl;
#endif
                dbrs_swid++;
              }
            }
          }
        }


        void ProcessAbstractReductionNetwork () {

          assert(num_levels_ >= 1);

          int num_adder_switches = num_mult_switches_ - 1;

          if (non_uniform) {
            std::vector<int> vn_sizes;
            std::ifstream readVNSizes("non_uniform_VN_sizes.txt");
            int size;
            int count = 0;

            while (readVNSizes >> size) {
              vn_sizes.push_back(size);
              if (size == 1) {
                count += 2;
              } else {
                count += size;
              }
              std::cout << size << "\n" << std::endl;
            }
            readVNSizes.close();
            if (count >= num_mult_switches_) {
              std::cout << "ERROR: Non-Uniform VN Sizes total exceeds the number of multiplier switches.";
              return;
            }

            int vn_id = 0;
            int index = 0;
            while (!vn_sizes.empty()) {
              int vn_size = vn_sizes.front();
              vn_sizes.erase(vn_sizes.begin());
              //std::cout << "VN Size: " << vn_size << std::endl;

              int index_inc = vn_size;
              for (int i = index; i < index + vn_size; i++) {
                auto compile_packet = std::make_shared<CompilePacket>(vn_id, vn_size, 1);
                std::cout<< "VN Size: " << vn_size << ", VN ID: " << vn_id << std::endl;
                if (i < 2) {
                  int vn_num_exist = single_reduction_switches_[num_levels_-1][0]->GetVN_Nums();
                  if (vn_num_exist == 0) {
                    single_reduction_switches_[num_levels_-1][0]->PutPacket(compile_packet, i % 2);
                    if (vn_size < 2) {
                      index_inc++;
                    }
                  } else if (vn_num_exist == 1) {
                    if (single_reduction_switches_[num_levels_-1][0]->CheckIfSameID(vn_id)) {
                      single_reduction_switches_[num_levels_-1][0]->PutPacket(compile_packet, i % 2);
                    } else {
                      std::cout << "ERROR: One Single Switch inputs 2 kind of VNs" << std::endl;
                      return;
                    }
                  }
                  //single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, 0);
                  //single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, 1);
                  inorder_single_reduction_swtiches.insert(std::make_pair(0, std::make_pair(num_levels_ - 1, 0)));
                } else if (i > num_mult_switches_-3) {
                  int vn_num_exist = single_reduction_switches_[num_levels_-1][1]->GetVN_Nums();
                  if (vn_num_exist == 0) {
                    single_reduction_switches_[num_levels_-1][1]->PutPacket(compile_packet, i % 2);
                    if (vn_size < 2) {
                      index_inc++;
                    }
                  } else if (vn_num_exist == 1) {
                    if (single_reduction_switches_[num_levels_-1][1]->CheckIfSameID(vn_id)) {
                      single_reduction_switches_[num_levels_-1][1]->PutPacket(compile_packet, i % 2);
                    } else {
                      std::cout << "ERROR: One Single Switch inputs 2 kind of VNs" << std::endl;
                      return;
                    }
                  }
                  //single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, 0);
                  //single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, 1);
                  inorder_single_reduction_swtiches.insert(std::make_pair(num_adder_switches - 1, std::make_pair(num_levels_ - 1, 1)));
                } else {
                  int dbrs_id = (i - 2)/4;
                  int port_id = (i - 2) % 4;
                  int inorder_id =  2 * (i / 2);
                  
                  std::cout << "dbrs_id: " << dbrs_id << ", port_id: " << port_id << ", inorder_id: " << inorder_id << std::endl; 

                  int vn_num_exist = double_reduction_switches_[num_levels_-1][dbrs_id]->GetVN_Nums();
                  std::cout << "vn_num_exist: " << vn_num_exist << std::endl;
                  if (vn_num_exist == 0) {
                    double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id);
                    
                    /*if (vn_size < 2) {
                      index_inc++;
                    }*/
                    std::cout << "DW on branch == 0" << std::endl;
                  } else if (vn_num_exist == 1) {
                    int free_ports = double_reduction_switches_[num_levels_-1][dbrs_id]->GetFreePorts();
                    std::cout << "Free_ports: " << free_ports << std::endl;
                    if (double_reduction_switches_[num_levels_-1][dbrs_id]->CheckIfSameID(vn_id)) {
                      std::cout << "DW on branch == 1-1" << std::endl;
                      double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id);
                    } else if (vn_size >= free_ports) { // this kind of vn first time entry, so the whole vn size have not being filled
                      std::cout << "DW on branch == 1-2" << std::endl;
                      double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id);
                    } else {
                      std::cout << "DW on branch == 1-3" << std::endl;
                      // Fill double switch with the second VN kind from right to left. This is to utilize the property of double switch.
                      for (int j = 0; j < vn_size; j++) {
                        double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id + free_ports - 1 - j);
                      }
                      index_inc += (free_ports - vn_size);
                      break;
                    } 
                  } else if (vn_num_exist == 2) {
                    if (double_reduction_switches_[num_levels_-1][dbrs_id]->CheckIfSameID(vn_id)) {
                      std::cout << "DW on branch == 2-1" << std::endl;
                      double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id);
                    } else {
                      std::cout << "ERROR: One Single Switch inputs 2 kind of VNs" << std::endl;
                      return;
                    }
                  }    
                  inorder_double_reduction_swtiches.insert(std::make_pair(inorder_id, std::make_pair(num_levels_ - 1, dbrs_id)));
                }
              }
              index += index_inc;
              vn_id++;
              std::cout << "Index add up: " << index_inc  << ", Index now: " << index << std::endl;
            }

            std::cout << "Final Index now: " << index << std::endl;

            if (index >= num_mult_switches_) {
              std::cout << "ERROR: Non-Uniform VN Sizes total exceeds the number of multiplier switches.";
              return;
            }

            IdleSwitchesProcess(index);
          } else {
            //int num_vns = vn_num_;

            // special case handle: vn_size = 1 (ps: vn_num cannot exceed num_multiplier / 2)
            if (vn_size_ == 1) {
              LowestLevelSingleVNCase();
              /*int max_vn_num = num_mult_switches_ / 2;
              if (num_vns > max_vn_num) {
                std::cout << "ERROR: Number of VNs exceeds the maximum number(num_multiplier / 2) allowed for VN SIZE 1" << std::endl;
                return;
              } else {
                for (int vn_id = 0; vn_id < max_vn_num; vn_id++) {
                  if (vn_id < num_vns) {
                    auto compile_packet = std::make_shared<CompilePacket>(vn_id, vn_size_, 1);
                    if (vn_id == 0) {
                      single_reduction_switches_[num_levels_-1][0]->PutPacket(compile_packet, 0);
                      //single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, 0);
                      //single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, 1);
                      inorder_single_reduction_swtiches.insert(std::make_pair(0, std::make_pair(num_levels_ - 1, 0)));
                    } else if (vn_id == max_vn_num - 1) {
                      single_reduction_switches_[num_levels_-1][1]->PutPacket(compile_packet, 0);
                      //single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, 0);
                      //single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, 1);
                      inorder_single_reduction_swtiches.insert(std::make_pair(num_adder_switches - 1, std::make_pair(num_levels_ - 1, 1)));
                    } else {
                      int dbrs_id = (vn_id - 1) / 2;
                      int pos_id = (vn_id - 1) % 2;
                      int port_id = pos_id == 0 ? 0 : 3;
                      int inorder_id = vn_id * 2;

                      double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id);
                      //double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id);
                      //double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id + 1);
                      inorder_double_reduction_swtiches.insert(std::make_pair(inorder_id, std::make_pair(num_levels_ - 1, dbrs_id)));
                    }
                  } else {
                    if (vn_id == 0) {
                      single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, 0);
                      single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, 1);
                      inorder_single_reduction_swtiches.insert(std::make_pair(0, std::make_pair(num_levels_ - 1, 0)));
                    } else if (vn_id == max_vn_num - 1) {
                      single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, 0);
                      single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, 1);
                      inorder_single_reduction_swtiches.insert(std::make_pair(num_adder_switches - 1, std::make_pair(num_levels_ - 1, 1)));
                    } else {
                      int dbrs_id = (vn_id - 1) / 2;
                      int pos_id = (vn_id - 1) % 2;
                      int port_id = pos_id == 0 ? 0 : 3;
                      int inorder_id = vn_id * 2;

                      int port_id_nearby = port_id == 0 ? 1 : 2;
                      double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id);
                      double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id_nearby);
                      inorder_double_reduction_swtiches.insert(std::make_pair(inorder_id, std::make_pair(num_levels_ - 1, dbrs_id)));
                    }
                  }
                }
              }*/

            } else {
              LowestLevelNormalCase();
              //Lowest level
              /*for(int inPrt = 0; inPrt < num_mult_switches_; inPrt++) {
                int vn_id = inPrt / vn_size_;
                //int num_vns = num_mult_switches_ / vn_size_;
                auto compile_packet = std::make_shared<CompilePacket>(vn_id, vn_size_, 1);

                //            for (int sw = 0; sw < num_mult_switches_; sw++) {
                if(vn_id < num_vns) {
                  if(inPrt <2) {
  #ifdef DEBUG
                    std::cout << "SGRS[" << num_levels_-1 << "][0] receives an initial packet to port " << inPrt %2 << std::endl;
  #endif
                    single_reduction_switches_[num_levels_-1][0]->PutPacket(compile_packet, inPrt %2 );
                    single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, inPrt % 2);
                    inorder_single_reduction_swtiches.insert(std::make_pair(0, std::make_pair(num_levels_ - 1, 0)));
                  }
                  else if(inPrt > num_mult_switches_-3) {
  #ifdef DEBUG
                    std::cout << "SGRS[" << num_levels_-1 << "][1] receives an initial packet to port " << inPrt %2 << std::endl;
  #endif
                    single_reduction_switches_[num_levels_-1][1]->PutPacket(compile_packet, inPrt %2 );
                    single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, inPrt % 2);
                    inorder_single_reduction_swtiches.insert(std::make_pair(num_adder_switches - 1, std::make_pair(num_levels_ - 1, 1)));
                  }
                  else {
                    int dbrs_id = (inPrt - 2)/4;
                    int port_id = (inPrt -2) % 4;
  #ifdef DEBUG
                    std::cout << "DBRS[" << num_levels_-1 << "]["<< dbrs_id << "] receives an initial packet to port " << port_id << std::endl;
  #endif
                    int inorder_id =  2 * (inPrt / 2);
                    double_reduction_switches_[num_levels_-1][dbrs_id]->PutPacket(compile_packet, port_id);
                    double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id);
                    std::cout << "inorder_id: " << inorder_id << ", dbrs_id: " << dbrs_id << ", port_id: " << port_id << std::endl;
                    inorder_double_reduction_swtiches.insert(std::make_pair(inorder_id, std::make_pair(num_levels_ - 1, dbrs_id)));
                  }
                } else {
                  if(inPrt <2) {
  #ifdef DEBUG
                    std::cout << "SGRS[" << num_levels_-1 << "][0] receives an initial packet to port " << inPrt %2 << std::endl;
  #endif
                    single_reduction_switches_[num_levels_-1][0]->SetIDConnect(-1, inPrt % 2);
                    inorder_single_reduction_swtiches.insert(std::make_pair(0, std::make_pair(num_levels_ - 1, 0)));
                  }
                  else if(inPrt > num_mult_switches_-3) {
  #ifdef DEBUG
                    std::cout << "SGRS[" << num_levels_-1 << "][1] receives an initial packet to port " << inPrt %2 << std::endl;
  #endif
                    single_reduction_switches_[num_levels_-1][1]->SetIDConnect(-1, inPrt % 2);
                    inorder_single_reduction_swtiches.insert(std::make_pair(num_adder_switches - 1, std::make_pair(num_levels_ - 1, 1)));
                  }
                  else {
                    int dbrs_id = (inPrt - 2)/4;
                    int port_id = (inPrt -2) % 4;
  #ifdef DEBUG
                    std::cout << "DBRS[" << num_levels_-1 << "]["<< dbrs_id << "] receives an initial packet to port " << port_id << std::endl;
  #endif
                    int inorder_id =  2 * (inPrt / 2);
                    std::cout << "inorder_id: " << inorder_id << ", dbrs_id: " << dbrs_id << ", port_id: " << port_id << std::endl;
                    double_reduction_switches_[num_levels_-1][dbrs_id]->SetIDConnect(-1, port_id);
                    inorder_double_reduction_swtiches.insert(std::make_pair(inorder_id, std::make_pair(num_levels_ - 1, dbrs_id)));
                  }
                }
                //}
              }
  #ifdef DEBUG
            std::cout << std::endl;
  #endif*/
            }
          }

          //Process rest of the levels from the lowest level
          for(int lv = num_levels_-1; lv >= 0 ; lv--) {
            //Process packets within switches
            for(auto sgrs : single_reduction_switches_[lv]) {
              sgrs->ProcessPackets();
            }

            for(auto dbrs : double_reduction_switches_[lv]) {
              dbrs->ProcessPackets();
            }

            //Forward output packets to the upper level
            if(lv > 0) {
              /* Interconnect SGRSes */
              auto sgrs0_output = single_reduction_switches_[lv][0]->GetPacket(0);
              int reverse_level_prev = num_levels_ - 1 - (lv - 1);
              int reverse_level = num_levels_ - 1 - lv;
              int pos;
              int id_to_connect = pow(2, reverse_level) - 1;
              //inorder_single_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv, 0)));
              if(sgrs0_output != nullptr) {
#ifdef DEBUG
                std::cout << "SGRS[" << lv << "][0] Sends a packet to SGRS[" << lv-1 << "][0]" << std::endl;
#endif
                single_reduction_switches_[lv-1][0]->PutPacket(sgrs0_output, 0);
                pos = pow(2, reverse_level_prev) - 1;
                inorder_single_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv - 1, 0)));
                single_reduction_switches_[lv-1][0]->SetIDConnect(id_to_connect, 0);
              }

              auto sgrs1_output = single_reduction_switches_[lv][1]->GetPacket(0);
              id_to_connect = pow(2, reverse_level) - 1 + 2 * (pow(2, lv) - 1) * pow(2, reverse_level);
              //pos = pow(reverse_level_prev, 2) - 1 + 2 * pow(lv, 2) * pow(reverse_level_prev, 2);
              //inorder_single_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv, 1)));
              if(sgrs1_output != nullptr) {
                if(lv != 1) {
#ifdef DEBUG
                  std::cout << "SGRS[" << lv << "][1] Sends a packet to SGRS[" << lv-1 << "][1]" << std::endl;
#endif
                  single_reduction_switches_[lv-1][1]->PutPacket(sgrs1_output, 1);
                  pos = pow(2, reverse_level_prev) - 1 + 2 * (pow(2, lv - 1) - 1) * pow(2, reverse_level_prev);
                  inorder_single_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv - 1, 1)));
                  single_reduction_switches_[lv-1][1]->SetIDConnect(id_to_connect, 1);
                }
                else {
#ifdef DEBUG
                  std::cout << "SGRS[" << lv << "][1] Sends a packet to SGRS[" << lv-1 << "][0]" << std::endl;
#endif                  
                  single_reduction_switches_[lv-1][0]->PutPacket(sgrs1_output, 1);
                  pos = pow(2, reverse_level_prev) - 1 + 2 * pow(lv, 2) * pow(2, reverse_level_prev);
                  inorder_single_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv - 1, 0)));
                  single_reduction_switches_[lv-1][0]->SetIDConnect(id_to_connect, 1);
                }
              }


              int num_dbrs_in_lv = GetNumDBRS(lv);
              int num_dbrs_in_prev_lv = GetNumDBRS(lv-1);

              std::cout << std::endl;

              if(lv > 1) {
                auto dbrs_lEdgeOutput = double_reduction_switches_[lv][0]->GetPacket(0);
                id_to_connect = pow(2, reverse_level) - 1 + 2*1*pow(2, reverse_level);
                //inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv, 0)));

                //int pos_R = pow(reverse_level_prev, 2) - 1 + 2*2*pow(reverse_level_prev, 2);
                //inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv, 0)));
                if(dbrs_lEdgeOutput != nullptr) {
#ifdef DEBUG
                  std::cout << "DBRS[" << lv << "][0]" << "Sends a packet to " << "SGRS[ " << lv -1 << "][0]" << std::endl;
#endif
                  single_reduction_switches_[lv-1][0]->PutPacket(dbrs_lEdgeOutput, 1);
                  //pos = pow(reverse_level_prev, 2) - 1 + 2*1*pow(reverse_level_prev, 2);
                  //inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv - 1, 0)));
                  single_reduction_switches_[lv-1][0]->SetIDConnect(id_to_connect, 1);
                }

                auto dbrs_rEdgeOutput = double_reduction_switches_[lv][num_dbrs_in_lv-1]->GetPacket(1);
                //pos_L = pow(reverse_level_prev, 2) - 1 + 2*(2 * num_dbrs_in_lv - 1)*pow(reverse_level_prev, 2);
                //inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv, num_dbrs_in_lv-1)));

                id_to_connect = pow(2, reverse_level) - 1 + 2*(2 * num_dbrs_in_lv)*pow(2, reverse_level);
                //inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv, num_dbrs_in_lv-1)));
                if(dbrs_rEdgeOutput != nullptr) {
#ifdef DEBUG
                  std::cout << "DBRS[" << lv << "][" << num_dbrs_in_lv-1 << "]" << "Sends a packet to " << "SGRS[ " << lv -1 << "][1]" << std::endl;
                  std::cout <<"Packet: vnID: " << dbrs_rEdgeOutput->GetVNID() << ", vnSz: " << dbrs_rEdgeOutput->GetVNSize() << ", pSum: " << dbrs_rEdgeOutput->GetNumPSums() << std::endl;
#endif
                  single_reduction_switches_[lv-1][1]->PutPacket(dbrs_rEdgeOutput, 0);
                  //pos = pow(reverse_level_prev, 2) - 1 + 2*(2 * num_dbrs_in_lv)*pow(reverse_level_prev, 2);
                  //inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv - 1, num_dbrs_in_lv-1)));
                  single_reduction_switches_[lv-1][1]->SetIDConnect(id_to_connect, 0);
                }

                std::cout << std::endl;

                if(lv>2) {
                  int num_dbrs_inPrt_in_prev_lv = num_dbrs_in_prev_lv * 4;

                  for(int dbrs_inPrt = 0; dbrs_inPrt < num_dbrs_inPrt_in_prev_lv; dbrs_inPrt++) {
                    int targ_input_sw_id = dbrs_inPrt/4;
                    int targ_input_sw_port = dbrs_inPrt % 4;
                    int targ_output_sw_id = (dbrs_inPrt+1)/2;
                    int targ_output_sw_port = (dbrs_inPrt+1) % 2;

                    auto packet = double_reduction_switches_[lv][targ_output_sw_id]->GetPacket(targ_output_sw_port);
                    id_to_connect = pow(2, reverse_level) - 1 + 2*(2 + dbrs_inPrt)*pow(2, reverse_level);
                    //inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv, targ_output_sw_id)));

                    //pos_R = pow(reverse_level_prev, 2) - 1 + 2*(2 + dbrs_inPrt)*pow(reverse_level_prev, 2);
                    //inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv, targ_output_sw_id)));
                    if(packet != nullptr && !(targ_output_sw_id == 0 && targ_output_sw_port == 0) ) {
#ifdef DEBUG
                      std::cout << "DBRS[" << lv << "][" << targ_output_sw_id << "] Sends a packet from port " << targ_output_sw_port << " to DBRS[" << lv-1 << "][" << targ_input_sw_id << "] port" << targ_input_sw_port << std::endl;
#endif
                      double_reduction_switches_[lv-1][targ_input_sw_id]->PutPacket(packet, targ_input_sw_port);
                      pos = pow(2, reverse_level_prev) - 1 + 2*(1 + targ_input_sw_id * 2 + targ_input_sw_port / 2)*pow(2, reverse_level_prev);
                      inorder_double_reduction_swtiches.insert(std::make_pair(pos, std::make_pair(lv - 1, targ_input_sw_id)));
                      double_reduction_switches_[lv-1][targ_input_sw_id]->SetIDConnect(id_to_connect, targ_input_sw_port);
                    }

                  } // End of for(int dbrs_inPrt = 0; dbrs_inPrt < num_dbrs_inPrt_in_prev_lv; dbrs_inPrt++)
                } // End of if (lv>2)
              } // End of if(lv>1)
            } // End of if(lv>0)
          } // end of for (lv = num_levels_-1; lv>=0; lv--)

          std::cout << "Finished processing reduction network information" << std::endl;

          /*int num_adder_switches = 0;
          for (int level = 0; level < num_levels_; level--) {
            num_adder_switches += pow(level, 2);
          }

          auto sgrs_config = this->GetSGRS_Config();
          auto dbrs_config = this->GetDBRS_Config();

          int sgrs_count = 0;
          for(auto it: *sgrs_config) {
            std::cout << "SGRS[" << sgrs_count << "]: " << it->ToString() << std::endl;
            if (sgrs_count == 0) {
              inorder_single_reduction_swtiches.insert(pair<int, int>(num_adder_switches / 2, 0));
            } else {
              int level = num_levels_ - (sgrs_count + 1) / 2 - 1;
              int pos = sgrs_count % 2;
              if (pos == 1) {
                int inorder_id = pow(level, 2) - 1;
                inorder_single_reduction_swtiches.insert(pair<int, int>(inorder_id, 0));
              } else {
                int inorder_id = pow(level, 2) - 1 + 2 * pos * pow(level, 2);
                inorder_single_reduction_swtiches.insert(pair<int, int>(inorder_id, 0));
              }
            }
            sgrs_count++;
          }

          int dbrs_count = 0;
          for(auto it: *dbrs_config) {
            std::cout << "DBRS[" << dbrs_count << "]: " << it->ToString() << std::endl;
            dbrs_count++;
          }*/
        }

        int GetNumDBRS(int target_level) {
          return static_cast<int>(pow(2, (target_level-1))) -1;
        }

        void PrintConfig() {
          int lv = 0;
          auto sgrs_config = this->GetSGRS_Config();
          auto dbrs_config = this->GetDBRS_Config();

          int sgrs_count = 0;
          for(auto it: *sgrs_config) {
            std::cout << "SGRS[" << sgrs_count << "]: " << it->ToString() << std::endl;
            sgrs_count++;
          }

          int dbrs_count = 0;
          for(auto it: *dbrs_config) {
            std::cout << "DBRS[" << dbrs_count << "]: " << it->ToString() << std::endl;
            dbrs_count++;
          }
        }

        std::shared_ptr<std::vector<std::shared_ptr<SGRS_Config>>> GetSGRS_Config() {
          std::shared_ptr<std::vector<std::shared_ptr<SGRS_Config>>> sgrs_configs = std::make_shared<std::vector<std::shared_ptr<SGRS_Config>>>();

          for(auto sgl_vec_it: single_reduction_switches_) {
            for(auto sgrs : sgl_vec_it) {
              auto config = std::make_shared<SGRS_Config>();
              config->genOutput_ = sgrs->GetGenOutput();
              config->mode_ = sgrs->GetMode();
              sgrs_configs->push_back(config);
            }
          }

          return sgrs_configs;
        }

        std::shared_ptr<std::vector<std::shared_ptr<DBRS_Config>>> GetDBRS_Config() {
          std::shared_ptr<std::vector<std::shared_ptr<DBRS_Config>>> dbrs_configs = std::make_shared<std::vector<std::shared_ptr<DBRS_Config>>>();

          for(auto dbl_vec_it: double_reduction_switches_) {
            for(auto dbrs : dbl_vec_it) {
              auto config = std::make_shared<DBRS_Config>();
              config->genOutputL_ = dbrs->GetGenOutputL();
              config->genOutputR_ = dbrs->GetGenOutputR();
              config->modeL_ = dbrs->GetModeL();
              config->modeR_ = dbrs->GetModeR();
              dbrs_configs->push_back(config);
            }
          }

          return dbrs_configs;
        }

        std::map<int, std::pair<int, int>> GetSGRS_Inorder_Map() {
          return inorder_single_reduction_swtiches;
        }

        std::map<int, std::pair<int, int>> GetDBRS_Inorder_Map() {
          return inorder_double_reduction_swtiches;
        }

        std::vector<std::vector<std::shared_ptr<SingleReductionSwitch>>> GetSGRS_Switches() {
          return single_reduction_switches_;
        }

        std::vector<std::vector<std::shared_ptr<DoubleReductionSwitch>>> GetDBRS_Switches() {
          return double_reduction_switches_;
        }
        

    }; // End of class AbstractReductionNetwork
  }; // End of namespace ReductionNetwork
}; // End of namespace MAERI

#endif
