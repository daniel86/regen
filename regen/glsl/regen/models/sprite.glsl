
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
