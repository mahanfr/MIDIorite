#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#define BUFFER_SIZE 2048
#define SAMPLE_RATE 48000
#define SAMPLE_SIZE 32
#define CHANNELS 1

#define MAX_NOTE_EVENTS 20

#define ATTACK_TIME  0.005f
#define RELEASE_TIME 0.03f

#define SEMITONE2Freq(n) (pow(2, (float)n/12.0f) * 16.352)

typedef struct {
    bool isPlaying;
    bool isReleasing;
    float volume;
    float elapsed_time;
    float phase;
} NoteState;

typedef struct {
    float y;
    int duration;
    bool active;
} VisualNote;

const char* note_name[12] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#","G", "G#", "A", "A#", "B"
};

const int note_keys[12] = {
    KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_V,
    KEY_G, KEY_B, KEY_N, KEY_J, KEY_M, KEY_COMMA
};

float create_audio_frame(NoteState notes[12], int octive ,float amp) {
    float frame = 0;
    for (int i=0; i < 12; ++i) {
        if (notes[i].volume > 0.001f) {
            float freq = SEMITONE2Freq(((octive * 12) + i));
            notes[i].phase += 2.0f * PI * freq / SAMPLE_RATE;

            if (notes[i].phase > 2.0f * PI) {
                notes[i].phase -= 2.0f * PI;
            }

            float amplitude = amp * notes[i].volume;
            frame += sinf(notes[i].phase) * amplitude;
        }
    }
    return Clamp(frame, -1.0f, 1.0f);
    //return frame;
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

int main(void) {
    InitWindow(800, 600, "MIDI-Orite");
    InitAudioDevice();

    //Sound tick = LoadSound("assets/Tick.mp3");

    //PlaySound(tick);
    SetAudioStreamBufferSizeDefault(BUFFER_SIZE);
    float audio_buf[BUFFER_SIZE] = {0};
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, SAMPLE_SIZE, CHANNELS);
    PlayAudioStream(stream);

    int octive = 4;

    // bool playing_notes[12] = {0};
    // *****
    NoteState notes[12] = {0};

    int current_note_duration[12] = {0};
    int visNotesIndex[12] = {0};
    VisualNote vnote[12][MAX_NOTE_EVENTS] = {0};
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < MAX_NOTE_EVENTS; j++) {
            vnote[i][j].active = false;
        }
    }
    float smoothed_amp = 1.0f;
    SetTargetFPS(60);
    float dt = 0.0f;
    while (!WindowShouldClose()) {
        dt = GetFrameTime();
        for (int i = 0; i < 12; ++i) {
            if (IsKeyDown(note_keys[i])) {
                notes[i].isPlaying = true;
                current_note_duration[i] += 1;
            } else {
                notes[i].isPlaying = false;
                if (current_note_duration[i] > 0) {
                    if (visNotesIndex[i] < MAX_NOTE_EVENTS) {
                        vnote[i][visNotesIndex[i]].duration = current_note_duration[i];
                        vnote[i][visNotesIndex[i]].y = 0;
                        vnote[i][visNotesIndex[i]].active = true;
                        visNotesIndex[i] ++;
                    }
                    current_note_duration[i] = 0;
                }
            }
        }
        if (IsKeyPressed(KEY_UP)) {
            if (octive < 8) octive ++;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            if (octive > 0) octive --;
        }
        while (IsAudioStreamProcessed(stream)) {
            float sample_dt = 1.0f / SAMPLE_RATE;
            for (int i = 0; i < BUFFER_SIZE; i++) {
                for (int n = 0; n < 12; n++)
                    update_note_envelope(&notes[n], sample_dt);

                float total_volume = 0.0f;
                for (int n=0; n < 12; n++)
                    total_volume += notes[n].volume;

                float target_amp = (total_volume > 1.0f) ? (1.0f / total_volume) : 1.0f;

                float smooth_coeff = 1.0f - expf(-sample_dt / 0.003f);
                smoothed_amp += (target_amp - smoothed_amp) * smooth_coeff;
                audio_buf[i] = create_audio_frame(notes, octive, smoothed_amp);
            }
            UpdateAudioStream(stream, audio_buf, BUFFER_SIZE);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        for (int i = 0; i < 12; ++i) {
            Color note_color = ColorFromHSV((360.f/12.f) * i, 1.0f, 1.0f);
            int note_width_box = 20;
            int note_start_box = ((GetScreenWidth() / 24) * ((i * 2) + 1)) - (note_width_box / 2);

            if (current_note_duration[i] > 0) {
                Rectangle rec = (Rectangle) {
                    .x = note_start_box, .y = 0,
                    .width  = note_width_box,
                    .height = 10 * current_note_duration[i]
                };
                DrawRectanglePro(rec, Vector2Zero(), 0.f, note_color);
            }

            for (int j = 0; j < visNotesIndex[i]; j++) {
                if (vnote[i][j].active) {
                    vnote[i][j].y += 1;

                    if (vnote[i][j].y > 50) {
                        vnote[i][j].active = false;
                        // Shifting active notes
                        for (int k = j; k < visNotesIndex[i] - 1; k++) vnote[i][k] = vnote[i][k + 1];
                        visNotesIndex[i]--;
                        continue;
                    }

                    if (vnote[i][j].duration > 0) {
                        Rectangle rec = (Rectangle) {
                            .x = note_start_box, .y = 10 * vnote[i][j].y,
                            .width  = note_width_box,
                            .height = 10 * vnote[i][j].duration
                        };
                        DrawRectanglePro(rec, Vector2Zero(), 0.f, note_color);
                    }
                }
            }
        }
        DrawLineEx((Vector2) {.x=0,.y=500}, (Vector2) {.x=GetScreenWidth(),.y=500}, 1.2f, RAYWHITE);
        DrawRectangle(0, 500, GetScreenWidth(), GetScreenHeight() - 500, BLACK);

        for (int i = 0; i < 12; ++i) {
            char text[5] = {0};
            sprintf(text, "%s%d", note_name[i], octive);
            int note_width = (MeasureText(text, 30) / 2);
            Color note_color = ColorFromHSV((360.f/12.f) * i, 0.2f, 0.2f);
            if (notes[i].isPlaying) {
                note_color = ColorFromHSV((360.f/12.f) * i, 1.0f, 1.0f);
            }
            int note_start = ((GetScreenWidth() / 24) * ((i * 2) + 1)) - note_width;
            DrawText(text, note_start , GetScreenHeight() - 40, 30, note_color);
        }

        // UI
        char text[255] = {0};
        sprintf(text, "Octive: %d", octive);
        DrawText(text, 10, 10, 20, WHITE);
        EndDrawing();
    }

    //UnloadSound(tick);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
