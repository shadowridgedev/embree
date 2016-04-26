// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "intersection_filter.h"
#include "../common/tutorial/tutorial.h"

namespace embree
{
  extern "C" Mode g_mode = MODE_NORMAL;

  struct Tutorial : public TutorialApplication 
  {
    Tutorial() : TutorialApplication("intersection_filter") 
    {
      /* set default camera */
      camera.from = Vec3fa(1.4f,1.3f,-1.5f);
      camera.to   = Vec3fa(0.0f,0.0f,0.0f);

      /* register parsing of stream mode */
      registerOption("mode", [this] (Ref<ParseStream> cin, const FileName& path) {
          std::string mode = cin->getString();
          if      (mode == "normal"           ) g_mode = MODE_NORMAL;
          else if (mode == "stream-coherent"  ) g_mode = MODE_STREAM_COHERENT;
          else if (mode == "stream-incoherent") g_mode = MODE_STREAM_INCOHERENT;
          else throw std::runtime_error("invalid mode:" +mode);
        }, 
        "--mode: sets rendering mode\n"
        "  normal           : normal mode\n"
        "  stream-coherent  : coherent stream mode\n"
        "  stream-incoherent: incoherent stream mode\n");
    }
  };
}

int main(int argc, char** argv) {
  return embree::Tutorial().main(argc,argv);
}
