global R_2D_Vertex g_r_2d_vertex_nil;
global R_3D_Vertex g_r_3d_vertex_nil;
global R_2D_Rect g_r_2d_rect_nil;

inline internal R_Handle r_handle_zero() {
    return (R_Handle)0;
}

inline internal R_2D_Vertex r_2d_vertex(f32 x, f32 y, f32 u, f32 v, V4_F32 color) {
    R_2D_Vertex result = g_r_2d_vertex_nil;
    result.dst.x = x;
    result.dst.y = y;
    result.src.x = u;
    result.src.y = v;
    result.color = color;
    return result;
}

inline internal R_2D_Rect r_2d_rect(Rect dst, Rect src, V4_F32 color, f32 border_thickness, f32 omit_tex) {
    R_2D_Rect result = g_r_2d_rect_nil;
    result.dst = dst;
    result.src = src;
    result.color = color;
    result.border_thickness = border_thickness;
    result.omit_tex = omit_tex;
    return result;
}

inline internal R_3D_Vertex r_3d_vertex(V3_F32 pos, V4_F32 color, V2_F32 tex) {
    R_3D_Vertex result = g_r_3d_vertex_nil;
    result.pos = v4_f32(pos.x, pos.y, pos.z, 0);
    result.color = color;
    result.tex = tex;
    return result;
}
