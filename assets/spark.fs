#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform float time;
uniform float seed;
uniform vec3  pixelColor;

const float particleLifetime = 0.35;
const float particleSpeed = 2.0;
const float gravity = 2.2;

const int PARTICLES = 14;

float hash(float x) {
    return fract(sin(x) * 43758.5453123);
}

void main() {
    vec2 uv = fragTexCoord * 2.0 - 1.0;

    vec3 color = vec3(0.0);

    vec2 origin = vec2(0.0, 0.0);

    for(int i = 0; i < PARTICLES; i++) {
        float id = float(i);

        float offset = hash(id + seed) * particleLifetime;
        float age = mod(time + offset, particleLifetime);
        float life = 1.0 - age / particleLifetime;

        float r = hash(id + seed);
        float angle;
        if(r < 0.5) {
            // Left
            angle = radians(180.0) + (hash(id*11.0)-0.5) * 0.25;
        } else {
            // Right
            angle = (hash(id*11.0)-0.5) * 0.25;
        }

        float speed = mix(0.2, 2.0, hash(id*17.0));
        vec2 velocity = vec2(cos(angle), sin(angle));
        velocity *= speed * particleSpeed;
        velocity.y -= mix(0.8, 2, hash(id*23.0));

        vec2 pos = origin;
        pos += velocity * age;
        pos.y += gravity * age * age;
        float grid = 1.0 / 32.0;
        pos = floor(pos / grid) * grid;

        // Particle Size
        float radius = 0.045;
        vec2 delta = abs(uv - pos);
        float particle =
            step(delta.x, radius) *
            step(delta.y, radius);

        color = max(color, pixelColor * particle);
    }

    finalColor = vec4(color, 1.0);
}
