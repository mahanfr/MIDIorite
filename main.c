#include <bits/posix2_lim.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 2048
#define SAMPLE_RATE 48000
#define SAMPLE_SIZE 32
#define CHANNELS 1

#define KEYBOARD_NOTES 12
#define MAX_NOTE_EVENTS 20
#define NOTES_DESTROY_LINE 500

#define ATTACK_TIME  0.005f
#define RELEASE_TIME 0.03f

#define SEMITONE2Freq(n) (pow(2, (float)n/12.0f) * 16.352)

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

const char* note_key_names[KEYBOARD_NOTES] = {
    "Q", "W", "E", "R", "T", "Y",
    "U", "I", "O", "P", "[", "]"
};

// const int note_keys[KEYBOARD_NOTES] = {
//     KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_V,
//     KEY_G, KEY_B, KEY_N, KEY_J, KEY_M, KEY_COMMA
// };
const int note_keys[KEYBOARD_NOTES] = {
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y,
    KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET
};

typedef struct {
    AudioStream stream;
    float amplitude;
    int octive;
    int transpose;
    bool showKeyboardKeys;
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

void handle_input(AppState *app) {
    float dt = GetFrameTime();
    for (int i = 0; i < KEYBOARD_NOTES; ++i) {
        if (IsKeyDown(note_keys[i])) {
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
        app->transpose -= 6;
        if (app->transpose < -6) app->transpose = -6;
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        app->transpose += 6;
        if (app->transpose > 6) app->transpose = 6;
    }
    if (IsKeyPressed(KEY_TAB)) {
        app->showKeyboardKeys = !app->showKeyboardKeys;
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
            DrawRectanglePro(rec, Vector2Zero(), 0.f, note_color);
        }

        for (int j = 0; j < app->vnotesIndex[i]; j++) {
            if (app->vnotes[i][j].active) {
                app->vnotes[i][j].y += dt * 60; // TODO: Replace with const

                if (app->vnotes[i][j].y > 50) {
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
                    DrawRectanglePro(rec, Vector2Zero(), 0.f, note_color);
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
            NOTES_DESTROY_LINE - (sparkTexture.height/2.0f),
            sparkTexture.width, sparkTexture.height},
            Vector2Zero(), 0.0f, WHITE);
    EndShaderMode();
    EndBlendMode();
}

void draw_piano(AppState *app) {
    int totalSemitones = KEYBOARD_NOTES;
    const int whiteKeyCount = 7;
    
    float piano_height = GetScreenHeight() - NOTES_DESTROY_LINE;
    float pianoY = NOTES_DESTROY_LINE;
    float whiteKeyWidth = (float)GetScreenWidth() / whiteKeyCount;
    float blackKeyWidth =  whiteKeyWidth * 0.6f;
    float blackKeyHeight = piano_height * 0.6f;

    DrawRectangle(0, pianoY - 10, GetScreenWidth(), piano_height + 10, DARKBROWN);

    // First pass: Draw all white keys
    int whiteKeyIndex = 0;
    for (int i = 0; i < totalSemitones; i++) {
        const char* note = note_name[i];

        if (!strchr(note, '#')) {
            float x = whiteKeyIndex * whiteKeyWidth;
            Rectangle whiteKey = {x, pianoY, whiteKeyWidth - 1, piano_height};

            Color keyColor = (whiteKeyIndex % 2 == 0) ? WHITE : LIGHTGRAY;
            if (app->notes[i].isPlaying) {
                keyColor = ColorFromHSV((360.f/12.f) * i, 1.0f, 1.0f);
            }
            DrawRectangleRec(whiteKey, keyColor);

            // Draw note name
            int textWidth = MeasureText(note, 20);
            DrawText(note, x + whiteKeyWidth/2 - textWidth/2.0f, 
                    pianoY + piano_height - 30, 20, GRAY);

            whiteKeyIndex++;
        }
    }

    whiteKeyIndex = 0;
    for (int i = 0; i < totalSemitones; i++) {
        const char* note = note_name[i];

        if (strchr(note, '#')) {
            float x = whiteKeyIndex * whiteKeyWidth - blackKeyWidth / 2;

            Rectangle blackKey = {x, pianoY, blackKeyWidth, blackKeyHeight};
            Color keyColor = BLACK;
            if (app->notes[i].isPlaying) {
                keyColor = ColorFromHSV((360.f/12.f) * i, 1.0f, 1.0f);
            }
            DrawRectangleRec(blackKey, keyColor);

            // Draw note name
            int textWidth = MeasureText(note, 15);
            DrawText(note, x + blackKeyWidth/2 - textWidth/2.0, 
                    pianoY + blackKeyHeight - 25, 15, WHITE);
        } else {
            whiteKeyIndex++;
        }
    }

}

int main(void) {
    InitWindow(800, 600, "MIDI-Orite");
    InitAudioDevice();

    //Sound tick = LoadSound("assets/Tick.mp3");

    //PlaySound(tick);
    SetAudioStreamBufferSizeDefault(BUFFER_SIZE);
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    PlayAudioStream(stream);

    Image img = GenImageColor(128, 128, WHITE);
    Texture2D sparkTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    Shader spark_shader = LoadShader("assets/spark.vs", "assets/spark.fs");

    sparkShaderTimeLoc = GetShaderLocation(spark_shader, "time");
    sparkShaderSeedLoc = GetShaderLocation(spark_shader, "age");
    sparkShaderPixColorLoc = GetShaderLocation(spark_shader, "pixelColor");

    AppState app = {0};
    app.stream = stream;
    app.amplitude = 1;
    app.octive = 4;
    app.transpose = 0;

    SetTargetFPS(60);
    float dt = 0.0f;
    float age = 0.0f;
    while (!WindowShouldClose()) {
        dt = GetFrameTime();

        handle_input(&app);
        update_audio_stream(&app);

        BeginDrawing();
            ClearBackground(BLACK);
            draw_notes(&app);

            DrawLineEx(
                    (Vector2) {.x=0,.y=NOTES_DESTROY_LINE},
                    (Vector2) {.x=GetScreenWidth(),.y=NOTES_DESTROY_LINE},
                    1.2f, RAYWHITE);
            DrawRectangle(
                    0, NOTES_DESTROY_LINE, GetScreenWidth(),
                    GetScreenHeight() - NOTES_DESTROY_LINE, BLACK);

            // ---- Spark Shaders ---- //
            for (int i=0; i < KEYBOARD_NOTES; ++i) {
                int note_width_box = 20;
                int note_start_box = ((GetScreenWidth() / 24) * ((i * 2) + 1)) - (note_width_box / 2);
                age += GetFrameTime();
                if (app.vnotesIndex[i] == 0 && app.notes[i].note_down_time > 50) {
                    render_spark(spark_shader, sparkTexture, i, note_start_box, note_width_box, age);
                } else {
                    for (int j = app.vnotesIndex[i] - 1; j >= 0; j--) {
                        VisualNote vnote = app.vnotes[i][j];
                        float gravity_speed = 10;
                        if ((vnote.y + vnote.duration) * gravity_speed > NOTES_DESTROY_LINE) {
                            render_spark(spark_shader, sparkTexture, i, note_start_box, note_width_box, age);
                        }
                    }
                }
            }

            draw_piano(&app);

            // for (int i = 0; i < KEYBOARD_NOTES; i++) {
            //     int index = abs(i + app.transpose) % KEYBOARD_NOTES;
            //     bool isMinerNote = strlen(note_name[index]) == 2;
            //     if (isMinerNote) {
            //         Rectangle rec = (Rectangle) {key * i, NOTES_DESTROY_LINE, key, height * 0.65};
            //         DrawRectangleRec(rec, BLACK);
            //     }
            // }

            // for (int i = 0; i < KEYBOARD_NOTES; ++i) {
            //     int index = abs(i + app.transpose) % KEYBOARD_NOTES;
            //     bool isMinerNote = strlen(note_name[index]) == 2;

            //     // octave number
            //     int note_octive = app.octive;
            //     if (app.transpose > 0 && i > app.transpose) {
            //         note_octive ++;
            //     } else if (app.transpose < 0 && i < abs(app.transpose)) {
            //         note_octive --;
            //     }

            //     // Note name
            //     char text[5] = {0};
            //     if (!app.showKeyboardKeys) {
            //         sprintf(text, "%s%d", note_name[index], note_octive);
            //     } else {
            //         sprintf(text, "%s", note_key_names[i]);
            //     }

            //     // Note color
            //     Color note_color = ColorFromHSV((360.f/12.f) * i, 0.2f, 0.2f);
            //     if (app.notes[i].isPlaying) {
            //         note_color = ColorFromHSV((360.f/12.f) * i, 1.0f, 1.0f);
            //     }

            //     // font
            //     int fontSize = 30;
            //     int note_width = isMinerNote ? fontSize / 4 : MeasureText(text, fontSize);
            //     int half_note_offset = isMinerNote ? 15 : 0;



            //     // Vector2 pos = {0};
            //     // pos.x = ((GetScreenWidth() / 24.0f) * ((i*2) + 1)) - (note_width / 2.0f);
            //     // pos.y = GetScreenHeight() - 60 - half_note_offset;

            //     // float rotation = isMinerNote ? 90 : 0;

            //     // // draw note text
            //     // //DrawText(text, note_start , GetScreenHeight() - 60 - half_note_offset, fontSize, note_color);
            //     // DrawTextPro(GetFontDefault(), text, pos, Vector2Zero(), rotation ,fontSize, 1.0, note_color);
            // }

        EndDrawing();
    }

    //UnloadSound(tick);
    UnloadTexture(sparkTexture);
    UnloadAudioStream(app.stream);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
