#ifndef __NOISE_SETTINGS_HPP__
#define __NOISE_SETTINGS_HPP__

#include "opensimplex/OpenSimplexNoise.h"
#include "../../json/json.hpp"

namespace Components {
    class NoiseSettings {
        public:
        OpenSimplexNoise::Noise *noise;
        double base = 60.0f;
        double multiplier = 1.0f;
        double frequency = 0.03125f;
        double amplitude = 1.0f;
        double lacunarity = 1.8f;
        double gain = 0.75f;
        double easing = 1.0f;
        double min = 40.0f;
        double max = 200.0f;
        int octaves = 4;

        NoiseSettings(long long seed) {
            noise = new OpenSimplexNoise::Noise(seed);
        }

        void load(JSON::JSONObject& json) {
            if (json.Contains("base") && json["base"]->getType() == JSON::Type::Float) {
                base = json["base"]->getFloat();
            }
            if (json.Contains("multiplier") && json["multiplier"]->getType() == JSON::Type::Float) {
                multiplier = json["multiplier"]->getFloat();
            }
            if (json.Contains("easing") && json["easing"]->getType() == JSON::Type::Float) {
                easing = json["easing"]->getFloat();
            }
            if (json.Contains("min") && json["min"]->getType() == JSON::Type::Float) {
                min = json["min"]->getFloat();
            }
            if (json.Contains("max") && json["max"]->getType() == JSON::Type::Float) {
                max = json["max"]->getFloat();
            }
            if (json.Contains("frequency") && json["frequency"]->getType() == JSON::Type::Float) {
                frequency = json["frequency"]->getFloat();
            }
            if (json.Contains("amplitude") && json["amplitude"]->getType() == JSON::Type::Float) {
                amplitude = json["amplitude"]->getFloat();
            }
            if (json.Contains("lacunarity") && json["lacunarity"]->getType() == JSON::Type::Float) {
                lacunarity = json["lacunarity"]->getFloat();
            }
            if (json.Contains("gain") && json["gain"]->getType() == JSON::Type::Float) {
                gain = json["gain"]->getFloat();
            }
            if (json.Contains("octaves") && json["octaves"]->getType() == JSON::Type::Integer) {
                octaves = json["octaves"]->getInteger();
            }
        }

        double get(long long x, long long y, long long z) {
            double f = frequency;
            double a = amplitude;
            double sum = base;
            for (int i=0; i<octaves; i++) {
                sum += a * noise->eval(x*frequency, y*frequency, z*frequency);
                f *= lacunarity;
                a *= gain;
            }
            sum *= multiplier;
            if (sum < min) {
                sum = min;
            }
            if (sum > max) {
                sum = max;
            }
            sum = pow((sum - min) / (max - min), easing) * (max - min) + min;
            return sum;
        }

    };
}

#endif