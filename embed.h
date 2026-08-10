#ifndef HEADER_EMBED
#define HEADER_EMBED
const char* SparkVertexShader = "\
#version 330\n\
in vec3 vertexPosition;\n\
in vec2 vertexTexCoord;\n\
out vec2 fragTexCoord;\n\
uniform mat4 mvp;\n\
void main(){\n\
    fragTexCoord = vertexTexCoord;\n\
    gl_Position = mvp * vec4(vertexPosition, 1.0);\n\
}\n\
";
const char* SparkFragmentShader = "\n\
#version 330\n\
in vec2 fragTexCoord;\n\
out vec4 finalColor;\n\
uniform float time;\n\
uniform float seed;\n\
uniform vec3  pixelColor;\n\
const float particleLifetime = 0.35;\n\
const float particleSpeed = 2.0;\n\
const float gravity = 2.2;\n\
const int PARTICLES = 14;\n\
float hash(float x) {\n\
    return fract(sin(x) * 43758.5453123);\n\
}\n\
void main() {\n\
    vec2 uv = fragTexCoord * 2.0 - 1.0;\n\
    vec3 color = vec3(0.0);\n\
    vec2 origin = vec2(0.0, 0.0);\n\
    for(int i = 0; i < PARTICLES; i++) {\n\
        float id = float(i);\n\
        float offset = hash(id + seed) * particleLifetime;\n\
        float age = mod(time + offset, particleLifetime);\n\
        float life = 1.0 - age / particleLifetime;\n\
        float r = hash(id + seed);\n\
        float angle;\n\
        if(r < 0.5) {\n\
            angle = radians(180.0) + (hash(id*11.0)-0.5) * 0.25;\n\
        } else {\n\
            angle = (hash(id*11.0)-0.5) * 0.25;\n\
        }\n\
        float speed = mix(0.2, 2.0, hash(id*17.0));\n\
        vec2 velocity = vec2(cos(angle), sin(angle));\n\
        velocity *= speed * particleSpeed;\n\
        velocity.y -= mix(0.8, 2, hash(id*23.0));\n\
        vec2 pos = origin;\n\
        pos += velocity * age;\n\
        pos.y += gravity * age * age;\n\
        float grid = 1.0 / 32.0;\n\
        pos = floor(pos / grid) * grid;\n\
        float radius = 0.045;\n\
        vec2 delta = abs(uv - pos);\n\
        float particle =\n\
            step(delta.x, radius) *\n\
            step(delta.y, radius);\n\
        color = max(color, pixelColor * particle);\n\
    }\n\
    finalColor = vec4(color, 1.0);\n\
}\n\
";
#endif
