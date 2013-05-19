
-- getSpritePoints
vec3[4] getSpritePoints(vec3 p, vec2 size, vec3 upVector)
{
    vec3 zAxis = normalize(-p);
    vec3 xAxis = normalize(cross(zAxis, upVector));
    vec3 yAxis = normalize(cross(xAxis, zAxis));
    vec3 x = xAxis*0.5*size.x;
    vec3 y = yAxis*0.5*size.y;
    return vec3[](
        p - x - y,
        p - x + y,
        p + x - y,
        p + x + y
    );
}

-- getSpriteLayer
// Calculate cube map layers a sprite could affect.
// Should be done because a sprite can intersect with 3 cube faces at the same time.
int[3] getSpriteLayer(vec3 p)
{
    return int[](
        1 - int(sign(p.x)*0.5 + 0.5), //0 or 1
        3 - int(sign(p.y)*0.5 + 0.5), //2 or 3
        5 - int(sign(p.z)*0.5 + 0.5)  //4 or 5
    );
}

-- emit0
void emitSprite(mat4 proj, vec3 quadPos[4])
{
    out_spriteTexco = vec2(1.0,0.0);
    gl_Position = proj*vec4(quadPos[0],1.0);
    EmitVertex();

    out_spriteTexco = vec2(1.0,1.0);
    gl_Position = proj*vec4(quadPos[1],1.0);
    EmitVertex();

    out_spriteTexco = vec2(0.0,0.0);
    gl_Position = proj*vec4(quadPos[2],1.0);
    EmitVertex();

    out_spriteTexco = vec2(0.0,1.0);
    gl_Position = proj*vec4(quadPos[3],1.0);
    EmitVertex();
    EndPrimitive();
}

-- emit1
void emitSprite(mat4 proj, vec3 quadPos[4])
{
    out_spriteTexco = vec2(1.0,0.0);
    out_posEye = vec4(quadPos[0],1.0);
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(1.0,1.0);
    out_posEye = vec4(quadPos[1],1.0);
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,0.0);
    out_posEye = vec4(quadPos[2],1.0);
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,1.0);
    out_posEye = vec4(quadPos[3],1.0);
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    EndPrimitive();
}

-- emit2
void emitSprite(mat4 invView, mat4 proj, vec3 quadPos[4])
{
    out_spriteTexco = vec2(1.0,0.0);
    out_posEye = vec4(quadPos[0],1.0);
    out_posWorld = invView * out_posEye;
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(1.0,1.0);
    out_posEye = vec4(quadPos[1],1.0);
    out_posWorld = invView * out_posEye;
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,0.0);
    out_posEye = vec4(quadPos[2],1.0);
    out_posWorld = invView * out_posEye;
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,1.0);
    out_posEye = vec4(quadPos[3],1.0);
    out_posWorld = invView * out_posEye;
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    EndPrimitive();
}
