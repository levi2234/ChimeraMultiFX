#pragma once
#include "Effect.h"

// ============================================================
// Router — Multi-lane parallel signal routing
// ============================================================
//
//  [Input]                                      [Output]
//
//  IN 1 ──┬── Lane 0: [Comp → Dist → Cab ] ──── OUT 1
//          │
//  IN 2 ──┴── Lane 1: [Chorus → Delay    ] ──── OUT 2
//
// Each lane picks its input source and output destination.
// Lanes are processed in order so lane-to-lane feeds work.
// ============================================================

enum class InputSource {
    In_1,       // hardware input 1
    In_2,       // hardware input 2
    In_Mix,     // (in1 + in2) / 2
    Lane_0,     // output of another lane
    Lane_1,
    Lane_2,
    Lane_3,
};

enum class OutputDest {
    Out_1,      // hardware output 1 only
    Out_2,      // hardware output 2 only
    Out_Both,   // both outputs
    Out_None,   // muted — only used as feed for other lanes
};

class Router {
public:
    static constexpr int MAX_LANES = 4;
    static constexpr int MAX_SLOTS = 8;

    struct Lane {
        Effect*      slots[MAX_SLOTS] = {};
        int          count  = 0;
        float        level  = 1.0f;
        bool         active = false;
        InputSource  input  = InputSource::In_1;
        OutputDest   output = OutputDest::Out_Both;

        float        last_out = 0.f;

        void Add(Effect* fx) {
            if (count < MAX_SLOTS) { slots[count++] = fx; active = true; }
        }

        void Insert(int pos, Effect* fx) {
            if (count >= MAX_SLOTS || pos > count) return;
            for (int i = count; i > pos; i--) slots[i] = slots[i - 1];
            slots[pos] = fx;
            count++;
        }

        void Remove(int pos) {
            if (pos >= count) return;
            for (int i = pos; i < count - 1; i++) slots[i] = slots[i + 1];
            slots[--count] = nullptr;
            if (count == 0) active = false;
        }

        void Swap(int a, int b) {
            if (a < count && b < count) {
                Effect* tmp = slots[a];
                slots[a] = slots[b];
                slots[b] = tmp;
            }
        }

        void Clear() {
            for (int i = 0; i < count; i++) slots[i] = nullptr;
            count = 0; active = false;
        }

        float ProcessChain(float in) {
            float s = in;
            for (int i = 0; i < count; i++) {
                s = slots[i]->Tick(s);
            }
            last_out = s * level;
            return last_out;
        }
    };

    struct Output { float out1; float out2; };

    void Init(float sample_rate) {
        for (int r = 0; r < MAX_LANES; r++) {
            for (int i = 0; i < lanes[r].count; i++) {
                lanes[r].slots[i]->Init(sample_rate);
            }
        }
    }

    Output Process(float in1, float in2) {
        float out1 = 0.f, out2 = 0.f;
        bool any_output = false;

        for (int r = 0; r < MAX_LANES; r++) {
            if (!lanes[r].active) continue;

            float feed = ResolveInput(r, in1, in2);
            float s = lanes[r].ProcessChain(feed);

            switch (lanes[r].output) {
                case OutputDest::Out_1:
                    out1 += s;
                    any_output = true;
                    break;
                case OutputDest::Out_2:
                    out2 += s;
                    any_output = true;
                    break;
                case OutputDest::Out_Both:
                    out1 += s;
                    out2 += s;
                    any_output = true;
                    break;
                case OutputDest::Out_None:
                    break;
            }
        }

        if (!any_output) return {in1, in2};
        return {out1, out2};
    }

    void MoveEffect(int from_lane, int from_slot, int to_lane, int to_slot) {
        Effect* fx = lanes[from_lane].slots[from_slot];
        lanes[from_lane].Remove(from_slot);
        lanes[to_lane].Insert(to_slot, fx);
    }

    Lane lanes[MAX_LANES];

private:
    float ResolveInput(int lane_idx, float in1, float in2) {
        switch (lanes[lane_idx].input) {
            case InputSource::In_1:   return in1;
            case InputSource::In_2:   return in2;
            case InputSource::In_Mix: return (in1 + in2) * 0.5f;
            case InputSource::Lane_0: return lanes[0].last_out;
            case InputSource::Lane_1: return lanes[1].last_out;
            case InputSource::Lane_2: return lanes[2].last_out;
            case InputSource::Lane_3: return lanes[3].last_out;
            default:                  return in1;
        }
    }
};
