#define CATCH_CONFIG_MAIN

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <format>
#include <utility>
#include <string>
#include <random>

#include "include/common.hpp"
#include "include/renderer.hpp"
#include "include/svodag.hpp"
#include "include/formatter.hpp"

#define F std::format

// template <class... Args>
// constexpr std::string f(std::format_string<Args...> fmt, Args&&... args) {
// 	return (std::format(std::move(fmt), std::forward<Args&&>(args)...));
// }

TEST_CASE("Ensure that catch2 is working", "[meta]") {
	REQUIRE(true);
}

TEST_CASE( "Svo-Dag Construction", "[svodag]") {
	SvoDag svodag{};

	// svodag.insert(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	// CAPTURE(result.r, result.g, result.b, result.a);

	REQUIRE(svodag.get(glm::vec3(0.0f, 0.0f, 0.0f)) == 0);

	svodag.insert(
        glm::vec3(0.0f, 0.0f, 0.0f),
        1
	);

	INFO(F("{}", svodag.get(glm::vec3(0.0f, 0.0f, 0.0f))));
	REQUIRE(svodag.get(glm::vec3(0.0f, 0.0f, 0.0f)) == 1);
}

TEST_CASE("SVODAG insertion", "[svodag]") {
	SvoDag svodag{};

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(-1.0f, 1.0f);
	
	for (int i=0;i<1000;i++) {
		SECTION("One of the random tests...") {
			glm::vec3 pos = {dis(gen), dis(gen), dis(gen)};

			svodag.insert(pos, i+1);

			INFO(std::format("Expected: {}", i+1));
			INFO(std::format("Instead: {}", i+1));
			
			REQUIRE(svodag.get(pos) == i+1);
		}
	}
}

// TEST_CASE("SVODAG Serialization", "[svodag]") {
// 	SvoDag tree{3};
// 	auto asdf = tree.serialize();
// 	INFO(std::format("{}", asdf));

// 	REQUIRE(std::format("{}", asdf) == std::string("[Color: [vec4(0.000000, 0.000000, 0.000000, 0.000000)], Connected to: [0, 0, 0, 0, 0, 0, 0, 0], Color: [vec4(0.000000, 0.000000, 0.000000, 0.000000)], Connected to: [0, 0, 0, 0, 0, 0, 0, 0]]"));
// }

TEST_CASE("SVODAG insertion at x=0, y=128, z=128", "[svodag]") {
	SvoDag svodag{8};

	svodag.insert(0, 128, 128, 1);
	REQUIRE(svodag.get(0, 128, 128) == 1);
}

TEST_CASE("level_to_size function works properly", "[svodag] [util]") {
	REQUIRE(level_to_size(8, 8) == 1.0f);
	REQUIRE(level_to_size(7, 8) == 0.5f);
	REQUIRE(level_to_size(6, 8) == 0.25f);
}

TEST_CASE("Svodag query", "[svodag]") {
	SvoDag svodag{8};
	
	for (size_t x = 0; x < 256; x++) {
        for (size_t y = 0; y < 256; y++) {
            for (size_t z = 0; z < 256; z++) {
                size_t length = (x - 128) * (x - 128) + (y - 128) * (y - 128) +
                                (z - 128) * (z - 128);
                if (16383 < length && length < 16385) {
                    svodag.insert(
                        x, y, z,
                        1
                    );
                }
            }
        }
    }

	svodag.query(glm::vec3(0.0f, 0.0f, 0.0f));
}

TEST_CASE("Svodag deduplication", "[svodag]") {
    SvoDag svodag{8};

    MatID_t id = 0;
    for (size_t x = 0; x < 256; x++) {
        for (size_t y = 0; y < 256; y++) {
            for (size_t z = 0; z < 256; z++) {
                size_t length = (x - 128) * (x - 128) + (y - 128) * (y - 128) +
                                (z - 128) * (z - 128);
                if (16383 < length && length < 16385) {
                    svodag.insert(
                        x, y, z,
                        id++
                    );
                }
            }
        }
    }

    svodag.dedup();

    MatID_t id_comp = 0;
    for (size_t x = 0; x < 256; x++) {
        for (size_t y = 0; y < 256; y++) {
            for (size_t z = 0; z < 256; z++) {
                size_t length = (x - 128) * (x - 128) + (y - 128) * (y - 128) +
                                (z - 128) * (z - 128);
                if (16383 < length && length < 16385) {
                    REQUIRE(svodag.get(x, y, z) == id_comp++);
                }
            }
        }
    }
}

TEST_CASE("Svodag solidification", "[svodag]") {
    SvoDag svodag{3};

    for (size_t x = 0; x < 8; x++) {
        for (size_t y = 0; y < 8; y++) {
            for (size_t z = 0; z < 8; z++) {
                svodag.insert(x, y, z, 1);
            }
        }
    }

    svodag.dedup();

    std::vector<SerializedNode> data(svodag.serialize());

    REQUIRE(data.size() == 1);
    REQUIRE(data[0] == SerializedNode{1, {0, 0, 0, 0, 0, 0, 0, 0}});
}
