#pragma once

// ---------------------------------------------------------------------------
// GLSL sources, kept as raw string literals so the renderer is self-contained.
//
// In a bigger project you would put these in .vert/.frag files and pull them in
// through Qt's resource system (qt_add_resources + ":/shaders/node.vert").
//
// Both programs target OpenGL 3.3 core, which is what main() requests via
// QSurfaceFormat::setDefaultFormat().
// ---------------------------------------------------------------------------

namespace Shaders {

// --- edges ----------------------------------------------------------------
// Each edge is expanded on the CPU into two triangles.  The vertex shader
// offsets each corner along the edge normal by a *pixel* amount, so lines keep
// a constant on-screen thickness no matter how far you zoom in.
inline constexpr const char *edgeVertex = R"(#version 330 core
layout(location = 0) in vec2  aPos;        // world position of the edge endpoint
layout(location = 1) in vec2  aNormal;     // unit vector perpendicular to the edge
layout(location = 2) in float aHalfWidth;  // half thickness, in device pixels
layout(location = 3) in vec4  aColour;

uniform mat4  uMvp;
uniform float uPixelsPerUnit;              // world units -> pixels (i.e. the zoom)

out vec4 vColour;

void main()
{
    vec2 world = aPos + aNormal * (aHalfWidth / uPixelsPerUnit);
    gl_Position = uMvp * vec4(world, 0.0, 1.0);
    vColour = aColour;
}
)";

inline constexpr const char *edgeFragment = R"(#version 330 core
in  vec4 vColour;
out vec4 fragColour;

void main()
{
    fragColour = vec4(vColour.rgb * vColour.a, vColour.a);   // premultiplied
}
)";

// --- nodes ----------------------------------------------------------------
// One unit quad is drawn once per node using instancing: the per-vertex data is
// four corners, and everything that differs between nodes (centre, radius,
// colours) comes from per-instance attributes with a divisor of 1.
//
// The disc itself is produced in the fragment shader from the distance to the
// quad centre, which gives a perfectly smooth circle at any zoom for the cost
// of four vertices.
inline constexpr const char *nodeVertex = R"(#version 330 core
layout(location = 0) in vec2  aCorner;     // per-vertex:   (-1,-1) .. (1,1)
layout(location = 1) in vec2  aCentre;     // per-instance: world position
layout(location = 2) in float aRadius;     // per-instance: world radius
layout(location = 3) in vec4  aFill;
layout(location = 4) in vec4  aRing;
layout(location = 5) in float aRingWidth;  // 0 = no ring, else fraction of radius

uniform mat4 uMvp;

out vec2  vLocal;
out vec4  vFill;
out vec4  vRing;
out float vRingWidth;
out float vRadiusPx;

uniform float uPixelsPerUnit;

void main()
{
    // Pad the quad slightly so the antialiased rim is never clipped.
    vec2 corner = aCorner * 1.08;
    gl_Position = uMvp * vec4(aCentre + corner * aRadius, 0.0, 1.0);
    vLocal      = corner;
    vFill       = aFill;
    vRing       = aRing;
    vRingWidth  = aRingWidth;
    vRadiusPx   = aRadius * uPixelsPerUnit;
}
)";

inline constexpr const char *nodeFragment = R"(#version 330 core
in  vec2  vLocal;
in  vec4  vFill;
in  vec4  vRing;
in  float vRingWidth;
in  float vRadiusPx;

out vec4 fragColour;

void main()
{
    float d = length(vLocal);                  // 0 at the centre, 1 at the rim

    // One pixel expressed in the same units as d, so the edge is exactly one
    // pixel of antialiasing wide regardless of zoom.
    float aa = max(fwidth(d), 1.0 / max(vRadiusPx, 1.0));

    float disc = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, d);
    if (disc <= 0.0)
        discard;

    vec4 colour = vFill;
    if (vRingWidth > 0.0) {
        float inner = 1.0 - vRingWidth;
        float ring  = smoothstep(inner - aa, inner + aa, d);
        colour = mix(vFill, vRing, ring);
    }

    float alpha = colour.a * disc;
    fragColour = vec4(colour.rgb * alpha, alpha);   // premultiplied
}
)";

} // namespace Shaders
