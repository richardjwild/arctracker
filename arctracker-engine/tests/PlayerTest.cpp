#include "gtest/gtest.h"

extern "C"
{
#include "player/player.h"
#include "player/module.h"
#include "player/period.h"
#include "midi/midi_cmd_queue.h"
#include "ui/player_event_queue.h"
}

namespace {
    class PlayerTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            //
            // Create the player.
            //
            constexpr float sample_rate = 48000.0f;
            player = player_create();
            ASSERT_NE(nullptr, player);
            periods_init(PERIOD_MAX);
            //
            // Create the test module.
            //
            module = module_create(1, 1, 1, 1);
            ASSERT_NE(nullptr, module);
            module_init(module, 1, 60, 60);
            // BPM 60 with 60 lines per beat means each line will take 1.0s to play.
            module->master_gain = 1.0f; // So that the samples are played back with unity gain.
            module->tracks[0].panning = 1; // Panned fully left.
            auto *sample_data = static_cast<float *>(malloc(6 * sizeof(float)));
            sample_data[0] = 1.0f;
            sample_data[1] = 0.0f;
            sample_data[2] = 0.0f;
            sample_data[3] = -1.0f;
            sample_data[4] = 0.0f; // We always add two extra samples at the end
            sample_data[5] = 0.0f; // so that the interpolation and sample loops work correctly.
            constexpr int sample_index = 0;
            ASSERT_TRUE(module_set_sample(module, sample_data, 4, sample_rate, 0, 0, sample_index));
            instrument_t instrument = {};
            instrument.assigned = true;
            instrument.name[0] = '\0';
            instrument.default_volume = 255;
            instrument.transpose = 0;
            instrument.repeats = false;
            instrument.sample_index = sample_index;
            instrument.repeat_offset = 0;
            instrument.repeat_length = 0;
            module_set_instrument(module, 0, instrument);
            module_create_pattern(module, 0, 1);
            event_t event = {};
            event.note = 1;
            event.instrument_no = 1;
            module->patterns[0].events[0] = event;
            constexpr int sequence[] = {0};
            module_set_sequence(module, sequence, 1);
            //
            // Initialise the player.
            //
            midi_command_queue_t *midi_queue = midi_queue_init();
            player_event_queue_t *event_queue = event_queue_init();
            ASSERT_TRUE(player_init(player, module, event_queue, midi_queue, sample_rate));
        }

        void TearDown() override
        {
            player_destroy(player);
            module_destroy(module);
        }

        void play() const
        {
            player_command_t command = {};
            command.cmd_type = TOGGLE_PLAY;
            player_queue_command(player, command);
        }

        [[nodiscard]] std::array<float, 8> render() const
        {
            std::array<float, 8> output = {};
            player_render_audio(player, output.data(), 4, false);
            return output;
        }
        player_t *player = nullptr;
        module_t *module = nullptr;
    };

    TEST_F(PlayerTest, RendersNothingWhileWaitingToStart)
    {
        const auto output = render();
        for (const float sample : output)
        {
            EXPECT_FLOAT_EQ(0.0f, sample);
        }
    }

    TEST_F(PlayerTest, RendersSingleNoteAtUnityGain)
    {
        play();
        const auto output = render();
        constexpr std::array<float, 8> expected = {
            1.0f, 0.0f,
            0.0f, 0.0f,
            0.0f, 0.0f,
           -1.0f, 0.0f
        };
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_FLOAT_EQ(expected[i], output[i]) << "Sample index: " << i;
        }
    }
}
