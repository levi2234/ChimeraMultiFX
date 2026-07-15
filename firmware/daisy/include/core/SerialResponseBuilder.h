#pragma once

#include "Effect.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

class SerialResponseBuilder {
public:
    static void Append(char* out, int max, int& pos, const char* fmt, ...) {
        if (pos >= max - 1) return;
        va_list args;
        va_start(args, fmt);
        const int len = vsnprintf(out + pos, static_cast<size_t>(max - pos), fmt, args);
        va_end(args);
        if (len < 0) return;
        if (len >= max - pos) {
            pos = max - 1;
            out[pos] = '\0';
        } else {
            pos += len;
        }
    }

    static void AppendFloat(char* out, int max, int& pos, float value, int decimals) {
        if (pos >= max - 1) return;
        if (decimals < 0) decimals = 0;
        if (decimals > 4) decimals = 4;
        if (value < 0.0f) {
            Append(out, max, pos, "-");
            value = -value;
        }

        int scale = 1;
        for (int i = 0; i < decimals; i++) scale *= 10;
        const int scaled = static_cast<int>((value * scale) + 0.5f);
        const int whole = scaled / scale;
        const int fraction = scaled % scale;
        if (decimals == 0) Append(out, max, pos, "%d", whole);
        else Append(out, max, pos, "%d.%0*d", whole, decimals, fraction);
    }

    static void EmitParams(char* out, int max, int& pos, Effect* effect) {
        const char* list = effect->GetParamList();
        if (!list || list[0] == '\0') return;

        char names[128];
        strncpy(names, list, sizeof(names) - 1);
        names[sizeof(names) - 1] = '\0';
        bool first = true;
        char* token = strtok(names, ",");
        while (token) {
            if (!first) Append(out, max, pos, ",");
            Append(out, max, pos, "\"%s\":", token);
            AppendFloat(out, max, pos, effect->GetParam(token), 4);
            first = false;
            token = strtok(nullptr, ",");
        }
    }

    static void EmitParamInfo(char* out, int max, int& pos, Effect* effect) {
        bool first = true;
        for (int i = 0; i < effect->GetParamCount(); i++) {
            EffectParamInfo info;
            if (!effect->GetParamInfo(i, info)) continue;
            if (!first) Append(out, max, pos, ",");
            Append(out, max, pos,
                   "\"%s\":{\"label\":\"%s\",\"type\":\"%s\",\"unit\":\"%s\",\"scale\":\"%s\",\"min\":",
                   info.name, info.label, info.kind, info.unit, info.scale);
            AppendFloat(out, max, pos, info.min, 4);
            Append(out, max, pos, ",\"max\":");
            AppendFloat(out, max, pos, info.max, 4);
            Append(out, max, pos, ",\"default\":");
            AppendFloat(out, max, pos, info.default_value, 4);
            Append(out, max, pos, ",\"step\":");
            AppendFloat(out, max, pos, info.step, 4);
            if (info.options && info.options[0] != '\0') {
                char options[128];
                strncpy(options, info.options, sizeof(options) - 1);
                options[sizeof(options) - 1] = '\0';
                Append(out, max, pos, ",\"options\":[");
                bool first_option = true;
                char* option = strtok(options, ",");
                while (option) {
                    if (!first_option) Append(out, max, pos, ",");
                    Append(out, max, pos, "\"%s\"", option);
                    first_option = false;
                    option = strtok(nullptr, ",");
                }
                Append(out, max, pos, "]");
            }
            Append(out, max, pos, "}");
            first = false;
        }
    }
};