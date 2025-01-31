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

#include "abstract_reduction_network.hpp"
#include <memory>

#include<iostream>
#include<string>
#include<cstdlib>

#include "analysis-structure.hpp"
#include "parser.hpp"
#include "vmh_writer.hpp"

int main(int argc, char* argv[]) {

  if(argc != 6) {
    std::cout << "Usage: ./(ExeFile) (NumMultSwitches) (VNSize) (VNNum) (NonUniform) (LayerFileName)" << std::endl;
    return 0;
  }

  int numMultSwitches = atoi(argv[1]);
  int vn_size = atoi(argv[2]);
  int num_mapped_vns = atoi(argv[3]);
  bool non_uniform = atoi(argv[4]) == 0 ? false : true;

  auto ars = std::make_shared<MAERI::ReductionNetwork::AbstractReductionNetwork>(numMultSwitches, vn_size, num_mapped_vns, non_uniform);

  MAERI::MachineCodeGenerator::RNConfigWriter outputFileWriter("RN_Config.vmh");

  ars->ProcessAbstractReductionNetwork();
  //ars->PrintConfig();
  ars->PrintConfig_Inorder();

  auto dbrsConfig = ars->GetDBRS_Switches();
  auto mapping_DBRS = ars->GetDBRS_Inorder_Map();

  auto sgrsConfig = ars->GetSGRS_Switches();
  auto mapping_SGRS = ars->GetSGRS_Inorder_Map();

  int numLvs = static_cast<int>(log2(numMultSwitches));
  int num_adder_switches = numMultSwitches - 1;
  outputFileWriter.WriteVN_Config(dbrsConfig, sgrsConfig, mapping_DBRS, mapping_SGRS, numLvs, num_adder_switches);

  maestro::LayerParser layerParser(argv[5]);

  auto layerInfo = layerParser.ParseLayer();
  std::cout << "Parse finished" << std::endl;
  MAERI::MachineCodeGenerator::TileInfoWriter tileInfoWriter("Layer_Info.vmh");

  std::cout << layerInfo->ToString() << std::endl;

  tileInfoWriter.WriteTileInfo(layerInfo, numMultSwitches, vn_size, num_mapped_vns);

  return 0;
}
