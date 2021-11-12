#ifndef LAYOUTRTGEOMETRY_H
#define LAYOUTRTGEOMETRY_H

layout(binding = 2) buffer Pos {float p[]; }     pos[];  //non interleaved positions, normals and texture arrays
layout(binding = 3) buffer Nor {float n[]; }     nor[];
layout(binding = 4) buffer Tex {float t[]; }     tex[];
layout(binding = 5) buffer Ind {uint i[]; }  ind[];

layout(binding = 12) buffer Lights{Light l[]; } lights;
layout(binding = 13) buffer Materials{WaveFrontMaterialPacked m[]; } materials;
layout(binding = 14) buffer Instances{ObjectInstance i[]; } instances;

#endif //LAYOUTRTGEOMETRY_H