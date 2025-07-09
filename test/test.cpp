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

	glm::vec4 result = svodag.get(glm::vec3(0.0f, 0.0f, 0.0f));
	// CAPTURE(result.r, result.g, result.b, result.a);

	REQUIRE(svodag.get(glm::vec3(0.0f, 0.0f, 0.0f)) == glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

	svodag.insert(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec4(1.0f)
	);

	INFO(F("{}", svodag.get(glm::vec3(0.0f, 0.0f, 0.0f))));
	REQUIRE(svodag.get(glm::vec3(0.0f, 0.0f, 0.0f)) == glm::vec4(1.0f));
}

TEST_CASE("SVODAG insertion", "[svodag]") {
	SvoDag svodag{};

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(-1.0f, 1.0f);
	
	for (int i=0;i<1000;i++) {
		SECTION("One of the random tests...") {
			glm::vec3 pos = {dis(gen), dis(gen), dis(gen)};
			glm::vec4 color = {dis(gen), dis(gen), dis(gen), dis(gen)};

			svodag.insert(pos, color);

			INFO(std::format("Expected: {}", color));
			INFO(std::format("Instead: {}", svodag.get(pos)));
			
			REQUIRE(svodag.get(pos) == color);
		}
	}
}
