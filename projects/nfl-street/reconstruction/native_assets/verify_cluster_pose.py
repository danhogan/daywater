"""Independent original Skel/Clst reader for stored-pose inspection checks.

Uses host trigonometry, not the original polynomial. No animation input is
applied. Cluster parent selectors, not skeleton parents, drive this palette.
"""
import math
import struct

def cluster_palette(data, sections):
    skel, clst = sections[b'Skel'], sections[b'Clst']
    bones, names, translations, rotations, scales = struct.unpack_from('>5I',data,skel)
    rows,count=struct.unpack_from('>II',data,clst)
    assert 0<bones<=256 and 0<count<=256
    assert struct.unpack_from('>BBH',data,rows)==(0,255,4096)
    scales_by_bone=struct.unpack_from('>'+str(bones*3)+'f',data,scales)
    assert all(math.isfinite(s) for s in scales_by_bone)
    palette=[[[1.,0.,0.,0.],[0.,1.,0.,0.],[0.,0.,1.,0.],[0.,0.,0.,1.]]]
    for i in range(1,count):
        bone,parent,weight=struct.unpack_from('>BBH',data,rows+i*4)
        assert bone<bones and parent<i and weight<=4096
        a=[v*math.tau/65536 for v in struct.unpack_from('>3h',data,rotations+bone*6)]
        sx,sy,sz=map(math.sin,a);cx,cy,cz=map(math.cos,a)
        x,y,z=struct.unpack_from('>3f',data,translations+bone*16)
        local=[[cz*cy,-sz*cx+cz*sx*sy,sz*sx+cz*cx*sy,x],
               [sz*cy,cz*cx+sz*sx*sy,-cz*sx+sz*cx*sy,y],
               [-sy,cy*sx,cy*cx,z],[0.,0.,0.,1.]]
        # TMdl stores one XYZ scale vector per bone; compose the local
        # rotation with that scale before the parent cluster matrix.
        for r in range(3):
            for c in range(3):
                local[r][c] *= scales_by_bone[bone*3+c]
        # Full homogeneous concatenation provides an independent calculation.
        palette.append([[sum(palette[parent][r][k]*local[k][c] for k in range(4))
                         for c in range(4)] for r in range(4)])
    return palette
