#include "embedded/spark_vs.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <embedded/assets.h>
#define BUFFER_SIZE 2048
#define SAMPLE_RATE 48000
#define SAMPLE_SIZE 32
#define CHANNELS 1

#define KEYBOARD_NOTES 12
#define MAX_NOTE_EVENTS 20
#define PIANO_START_LINE (GetScreenHeight() * 0.75)

#define ATTACK_TIME  0.005f
#define RELEASE_TIME 0.03f

#define SEMITONE2Freq(n) (pow(2, (float)n/12.0f) * 16.352)

const char* help_text = "\
F1: Hide Help\n\
TAB: Show/Hide Keyboard Mappings\n\
R-Shift: Hold to increase octave\n\
R-Ctrl: Hold to decrease octave\n\
Left/Right arrows: transpose notes\n\
up/down arrows: change octave";

typedef struct {
    bool isPlaying;
    bool isReleasing;
    float volume;
    float elapsed_time;
    float note_down_time;
    float phase;
} NoteState;

typedef struct {
    float y;
    float duration;
    bool active;
} VisualNote;

const char* note_name[KEYBOARD_NOTES] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#","G", "G#", "A", "A#", "B"
};

const char* note_key_names[2][KEYBOARD_NOTES] = {
    { // With no Transpose
    "Z", "S", "X", "D", "C", "V",
    "G", "B", "H", "N", "J", "M"
    },
    { // With Transpose
     "A", "Z", "S", "X", "D", "C",
     "V", "G", "B", "H", "N", "M"
    }
};

const int note_keys[2][KEYBOARD_NOTES] = {
    { // With no Transpose
    KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_V,
    KEY_G, KEY_B, KEY_H, KEY_N, KEY_J, KEY_M
    },
    { // With Transpose
     KEY_A, KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C,
     KEY_V, KEY_G, KEY_B, KEY_H, KEY_N, KEY_M
    }
};

typedef struct {
    AudioStream stream;
    float amplitude;
    int octive;
    int transpose;
    bool showKeyboardKeys;
    bool showHelp;
    NoteState notes[KEYBOARD_NOTES];
    int vnotesIndex[KEYBOARD_NOTES];
    VisualNote vnotes[KEYBOARD_NOTES][MAX_NOTE_EVENTS];
    float audio_buf[BUFFER_SIZE];
} AppState;

float create_audio_frame(NoteState notes[KEYBOARD_NOTES], int octive, int transpose, float amp) {
    float frame = 0;
    for (int i=0; i < KEYBOARD_NOTES; ++i) {
        if (notes[i].volume > 0.001f) {
            int transposed_note = i + transpose;
            float freq = SEMITONE2Freq(((octive * KEYBOARD_NOTES) + transposed_note));
            notes[i].phase += 2.0f * PI * freq / SAMPLE_RATE;

            if (notes[i].phase > 2.0f * PI) {
                notes[i].phase -= 2.0f * PI;
            }

            float amplitude = amp * notes[i].volume;
            frame += sinf(notes[i].phase) * amplitude;
        }
    }
    return Clamp(frame, -1.0f, 1.0f);
}

void update_note_envelope(NoteState *note, float dt) {
    if (note->isPlaying) {
        if (note->isReleasing) {
            note->isReleasing = false;
            note->elapsed_time = 0.0f;
            note->volume = 0.0f;
        }
        note->elapsed_time += dt;
        note->volume = fminf(note->elapsed_time / ATTACK_TIME, 1.0f);
    } else {
        if (note->volume > 0.0f && !note->isReleasing) {
            note->isReleasing = true;
            note->elapsed_time = 0.0f;
        }
        if (note->isReleasing) {
            note->elapsed_time += dt;
            note->volume *= expf(-dt / RELEASE_TIME);
            if (note->volume < 0.001f) {
                note->volume = 0.0f;
                note->isReleasing = false;
                note->elapsed_time = 0.0f;
                note->phase = 0.0f;
            }
        }
    }
}

void update_audio_stream(AppState *app) {
    while (IsAudioStreamProcessed(app->stream)) {
        float sample_dt = 1.0f / SAMPLE_RATE;
        for (int i = 0; i < BUFFER_SIZE; i++) {
            for (int n = 0; n < KEYBOARD_NOTES; n++)
                update_note_envelope(&app->notes[n], sample_dt);

            float total_volume = 0.0f;
            for (int n=0; n < KEYBOARD_NOTES; n++)
                total_volume += app->notes[n].volume;

            float target_amp = (total_volume > 1.0f) ? (1.0f / total_volume) : 1.0f;

            float smooth_coeff = 1.0f - expf(-sample_dt / 0.003f);
            app->amplitude += (target_amp - app->amplitude) * smooth_coeff;
            app->audio_buf[i] = create_audio_frame(app->notes, app->octive, app->transpose, app->amplitude);
        }
        UpdateAudioStream(app->stream, app->audio_buf, BUFFER_SIZE);
    }
}

bool global_is_octive_up = false;
bool global_is_octive_down = false;
void handle_input(AppState *app) {
    float dt = GetFrameTime();
    int transpose_index = app->transpose == 0 ? 0 : 1;
    for (int i = 0; i < KEYBOARD_NOTES; ++i) {
        if (IsKeyDown(note_keys[transpose_index][i])) {
            app->notes[i].isPlaying = true;
            app->notes[i].note_down_time += dt*60; // TODO: Replace this with const
        } else {
            app->notes[i].isPlaying = false;
            if (app->notes[i].note_down_time > 0) {
                if (app->vnotesIndex[i] < MAX_NOTE_EVENTS) {
                    app->vnotes[i][app->vnotesIndex[i]].duration = app->notes[i].note_down_time;
                    app->vnotes[i][app->vnotesIndex[i]].y = 0;
                    app->vnotes[i][app->vnotesIndex[i]].active = true;
                    app->vnotesIndex[i] ++;
                }
                app->notes[i].note_down_time = 0;
            }
        }
    }
    if (IsKeyPressed(KEY_UP)) {
        if (app->octive < 8) app->octive ++;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        if (app->octive > 0) app->octive --;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        app->transpose -= KEYBOARD_NOTES / 2;
        if (app->transpose < -(KEYBOARD_NOTES / 2)) app->transpose = -(KEYBOARD_NOTES / 2);
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        app->transpose += KEYBOARD_NOTES / 2;
        if (app->transpose > KEYBOARD_NOTES / 2) app->transpose = KEYBOARD_NOTES / 2;
    }
    if (IsKeyPressed(KEY_TAB)) {
        app->showKeyboardKeys = !app->showKeyboardKeys;
    }
    if (IsKeyPressed(KEY_F1)) {
        app->showHelp = !app->showHelp;
    }
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        if (!global_is_octive_up && !global_is_octive_down) {
            app->octive++;
            global_is_octive_up = true;
        }
    } else {
        if (global_is_octive_up) {
            app->octive--;
            global_is_octive_up = false;
        }
    }
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (!global_is_octive_down && !global_is_octive_up) {
            app->octive--;
            global_is_octive_down = true;
        }
    } else {
        if (global_is_octive_down) {
            app->octive++;
            global_is_octive_down = false;
        }
    }
}

void draw_notes(AppState *app) {
    float dt = GetFrameTime();
    for (int i = 0; i < KEYBOARD_NOTES; ++i) {
        Color note_color = ColorFromHSV((360.f/12.f) * i, 1.0f, 1.0f);
        int note_width_box = 20;
        int note_start_box = ((GetScreenWidth() / 24) * ((i * 2) + 1)) - (note_width_box / 2);

        if (app->notes[i].note_down_time > 0) {
            Rectangle rec = (Rectangle) {
                .x = note_start_box, .y = 0,
                .width  = note_width_box,
                .height = 10 * app->notes[i].note_down_time
            };
            //DrawRectanglePro(rec, Vector2Zero(), 0.f, note_color);
            DrawRectangleRounded(rec, 1, 20, note_color);
        }

        for (int j = 0; j < app->vnotesIndex[i]; j++) {
            if (app->vnotes[i][j].active) {
                app->vnotes[i][j].y += dt * 60; // TODO: Replace with const

                if (app->vnotes[i][j].y > PIANO_START_LINE / 10) {
                    app->vnotes[i][j].active = false;
                    // Shifting active notes
                    for (int k = j; k < app->vnotesIndex[i] - 1; k++) app->vnotes[i][k] = app->vnotes[i][k + 1];
                    app->vnotesIndex[i]--;
                    continue;
                }

                if (app->vnotes[i][j].duration > 0) {
                    Rectangle rec = (Rectangle) {
                        .x = note_start_box, .y = 10 * app->vnotes[i][j].y,
                        .width  = note_width_box,
                        .height = 10 * app->vnotes[i][j].duration
                    };
                    //DrawRectanglePro(rec, Vector2Zero(), 0.f, note_color);
                    DrawRectangleRounded(rec, 1, 20, note_color);
                }
            }
        }
    }
}

int sparkShaderTimeLoc;
int sparkShaderSeedLoc;
int sparkShaderPixColorLoc;
void render_spark(Shader spark_shader,
        Texture sparkTexture,
        int index, float x,
        float width, float age) {
    int seed = GetRandomValue(0, 30000);
    Color note_color = ColorFromHSV((360.f/12.f) * index, 1.0f, 1.0f);
    Vector4 pcwa = ColorNormalize(note_color);
    Vector3 pixel_color = (Vector3) {pcwa.x, pcwa.y, pcwa.z};

    SetShaderValue(spark_shader, sparkShaderTimeLoc, &age, SHADER_UNIFORM_FLOAT);
    SetShaderValue(spark_shader, sparkShaderSeedLoc, &seed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(spark_shader, sparkShaderPixColorLoc, &pixel_color, SHADER_UNIFORM_VEC3);

    BeginBlendMode(BLEND_ADDITIVE);
    BeginShaderMode(spark_shader);
    DrawTexturePro(
            sparkTexture,
            (Rectangle) {0,0,sparkTexture.width,sparkTexture.height},
            (Rectangle) {
            x - (sparkTexture.width/2.0f) + (width/2.0f),
            PIANO_START_LINE - (sparkTexture.height/2.0f),
            sparkTexture.width, sparkTexture.height},
            Vector2Zero(), 0.0f, WHITE);
    EndShaderMode();
    EndBlendMode();
}
void draw_piano_keys(AppState *app, bool is_black) {
    const int whiteKeyCount = 7;
    float piano_height = GetScreenHeight() - PIANO_START_LINE;
    float pianoY = PIANO_START_LINE;
    float whiteKeyWidth = (float)GetScreenWidth() / whiteKeyCount;
    float blackKeyWidth =  whiteKeyWidth * 0.6f;
    float blackKeyHeight = piano_height * 0.6f;

    int whiteKeyIndex = 0;
    for (int i = 0; i < KEYBOARD_NOTES; i++) {
        int note_index = (i + app->transpose) % KEYBOARD_NOTES;
        if (note_index < 0) note_index += KEYBOARD_NOTES;
        const char* note = note_name[note_index];
        int transpose_index = app->transpose == 0 ? 0 : 1;

        int octave = app->octive;
        int semitone_offset = i + app->transpose;
        if (semitone_offset < 0) octave -= (abs(semitone_offset) / KEYBOARD_NOTES) + 1;
        else                     octave += semitone_offset / KEYBOARD_NOTES;

        bool is_sharp = strchr(note, '#') != 0;
        bool should_render = is_black ? is_sharp : !is_sharp;

        if (should_render) {
            float x;
            if (is_black) {
                x = whiteKeyIndex * whiteKeyWidth - blackKeyWidth / 2;
                Rectangle keyRect = {x, pianoY, blackKeyWidth, blackKeyHeight};
                Color keyColor = BLACK;
                if (app->notes[i].isPlaying) {
                    keyColor = ColorFromHSV((360.f/12.f) * i, 1.0f, 1.0f);
                }
                DrawRectangleRec(keyRect, keyColor);

                if (app->showKeyboardKeys) {
                    const char* key_name = note_key_names[transpose_index][i];
                    int textWidth = MeasureText(key_name, 15);
                    DrawText(key_name, (x + blackKeyWidth/2.0) - textWidth/2.0f,
                            pianoY + blackKeyHeight - 45, 15, WHITE);
                }

                const char* text = TextFormat("%s%d", note, octave);
                int textWidth = MeasureText(note, 20);
                DrawText(text, (x + blackKeyWidth/2.0) - textWidth/2.0,
                        pianoY + blackKeyHeight - 25, 15, WHITE);
            } else {
                x = whiteKeyIndex * whiteKeyWidth;
                Rectangle keyRect = {x, pianoY, whiteKeyWidth - 1, piano_height};

                Color keyColor = (whiteKeyIndex % 2 == 0) ? WHITE : LIGHTGRAY;
                if (app->notes[i].isPlaying) {
                    keyColor = ColorFromHSV((360.f/12.f) * i, 1.0f, 1.0f);
                }
                DrawRectangleRec(keyRect, keyColor);

                if (app->showKeyboardKeys) {
                    const char* key_name = note_key_names[transpose_index][i];
                    int textWidth = MeasureText(key_name, 18);
                    DrawText(key_name, x + whiteKeyWidth/2 - textWidth/2.0f,
                            pianoY + piano_height - 50, 18, DARKGRAY);
                }

                const char* text = TextFormat("%s%d", note, octave);
                int textWidth = MeasureText(text, 20);
                DrawText(text, x + whiteKeyWidth/2 - textWidth/2.0f,
                        pianoY + piano_height - 30, 20, GRAY);
            }
        }

        if (!is_sharp) {
            whiteKeyIndex++;
        }
    }
}

void draw_piano(AppState *app) {
    float piano_height = GetScreenHeight() - PIANO_START_LINE;
    float pianoY = PIANO_START_LINE;

    DrawRectangle(0, pianoY - 10, GetScreenWidth(), piano_height + 10, DARKBROWN);

    // First pass: Draw all white keys
    draw_piano_keys(app, false);
    // Second pass: Draw all black keys
    draw_piano_keys(app, true);
}

int main(void) {
    InitWindow(1080, 800, "MIDI-Orite");
    InitAudioDevice();

    //Sound tick = LoadSound("assets/Tick.mp3");

    //PlaySound(tick);
    SetAudioStreamBufferSizeDefault(BUFFER_SIZE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    PlayAudioStream(stream);

    Image img = GenImageColor(128, 128, WHITE);
    Texture2D sparkTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    Shader spark_shader = LoadShaderFromMemory((const char*)embedded_spark_vs, (const char*)embedded_spark_fs);

    sparkShaderTimeLoc = GetShaderLocation(spark_shader, "time");
    sparkShaderSeedLoc = GetShaderLocation(spark_shader, "age");
    sparkShaderPixColorLoc = GetShaderLocation(spark_shader, "pixelColor");

    AppState app = {0};
    app.stream = stream;
    app.amplitude = 1;
    app.octive = 4;
    app.transpose = 0;

    SetTargetFPS(60);
    float age = 0.0f;
    while (!WindowShouldClose()) {
        handle_input(&app);
        update_audio_stream(&app);

        BeginDrawing();
            ClearBackground(BLACK);
            draw_notes(&app);

            DrawLineEx(
                    (Vector2) {.x=0,.y=PIANO_START_LINE},
                    (Vector2) {.x=GetScreenWidth(),.y=PIANO_START_LINE},
                    1.2f, RAYWHITE);
            DrawRectangle(
                    0, PIANO_START_LINE, GetScreenWidth(),
                    GetScreenHeight() - PIANO_START_LINE, BLACK);

            // ---- Spark Shaders ---- //
            for (int i=0; i < KEYBOARD_NOTES; ++i) {
                int note_width_box = 20;
                int note_start_box = ((GetScreenWidth() / 24) * ((i * 2) + 1)) - (note_width_box / 2);
                age += GetFrameTime();
                if (app.vnotesIndex[i] == 0 && app.notes[i].note_down_time > PIANO_START_LINE / 10) {
                    render_spark(spark_shader, sparkTexture, i, note_start_box, note_width_box, age);
                } else {
                    for (int j = app.vnotesIndex[i] - 1; j >= 0; j--) {
                        VisualNote vnote = app.vnotes[i][j];
                        float gravity_speed = 10;
                        if ((vnote.y + vnote.duration) * gravity_speed > PIANO_START_LINE) {
                            render_spark(spark_shader, sparkTexture, i, note_start_box, note_width_box, age);
                        }
                    }
                }
            }

            draw_piano(&app);

            // UI
            if (app.showHelp) {
                DrawText(help_text, 10, 10, 20, RAYWHITE);
            } else {
                DrawText(TextFormat("F1: Help"), 10, 10, 20, RAYWHITE);
            }

        EndDrawing();
    }

    //UnloadSound(tick);
    UnloadTexture(sparkTexture);
    UnloadAudioStream(app.stream);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
