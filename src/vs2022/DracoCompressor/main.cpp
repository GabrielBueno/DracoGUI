#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include <SDL2/SDL.h>

#include <draco/compression/encode.h>
#include <draco/io/file_utils.h>
#include <draco/io/mesh_io.h>
#include <draco/io/stdio_file_reader.h>
#include <draco/io/stdio_file_writer.h>
#include <draco/io/file_reader_factory.h>
#include <draco/io/file_writer_factory.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <raylib/raylib.h>

enum struct EncodeResultType { Ok, FileReadErr, MeshEncodeErr, FileWriteErr };

struct EncodeResult {
	EncodeResultType type;
	draco::Status status;
};

struct Options {
	int pos_quantization_bits;
	int tex_coords_quantization_bits;
	int normals_quantization_bits;
	int generic_quantization_bits;
	int compression_level;
};

EncodeResult Encode(const std::string& input, const std::string& output, Options options) {
	auto meshread = draco::ReadMeshFromFile(input);
	auto meshptr  = std::move(meshread);

	if (!meshptr.ok())
		return { EncodeResultType::FileReadErr, meshptr.status() };

	auto mesh = meshptr.value().get();
	auto speed = 10 - options.compression_level;

	draco::Encoder       encoder;
	draco::EncoderBuffer buffer;

	encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, options.pos_quantization_bits);
	encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, options.tex_coords_quantization_bits);
	encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, options.normals_quantization_bits);
	encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, options.generic_quantization_bits);
	encoder.SetSpeedOptions(speed, speed);

	const draco::Status encodeStatus = encoder.EncodeMeshToBuffer(*mesh, &buffer);

	if (!encodeStatus.ok())
		return { EncodeResultType::MeshEncodeErr, encodeStatus };

	auto writeok = draco::WriteBufferToFile(buffer.data(), buffer.size(), output);

	if (!writeok)
		return { EncodeResultType::FileWriteErr };

	return { EncodeResultType::Ok };
}

int main(int argc, char **argv) {
	int initResult = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);

	if (initResult != 0) {
		fprintf(stderr, "failed to init SDL: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"DRACO GUI",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		1280,
		720,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
	);

	if (window == nullptr) {
		fprintf(stderr, "failed to create window: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

	if (renderer == nullptr) {
		fprintf(stderr, "faiuled to create renderer: %s\n", SDL_GetError());
		return -1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);

	//raylib
	InitWindow(800, 800, "raylib [3d viewer]");

	bool done = false;
	while (!done && !WindowShouldClose()) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			ImGui_ImplSDL2_ProcessEvent(&e);

			if (e.type == SDL_QUIT)
				done = true;

			if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		ImGui_ImplSDLRenderer_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		bool show = true;
		ImGui::ShowDemoWindow(&show);

		//raylib
		BeginDrawing();
		ClearBackground(RAYWHITE);
		DrawText("HELLLLO", 20, 20, 10, BLACK);
		EndDrawing();

		ImGui::Render();
		SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
		SDL_RenderPresent(renderer);
	}

	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	CloseWindow();

	return 0;
}

void TestEncoding() {
	Options options;

	draco::FileReaderFactory::RegisterReader(draco::StdioFileReader::Open);
	draco::FileWriterFactory::RegisterWriter(draco::StdioFileWriter::Open);

	options.pos_quantization_bits = 11;
	options.tex_coords_quantization_bits = 10;
	options.normals_quantization_bits = 8;
	options.generic_quantization_bits = 8;
	options.compression_level = 7;

	const char* input = "D:/Programacao/Projetos/3D/DracoCompressor/assets/dunklowred.obj";
	const char* output = "D:/Programacao/Projetos/3D/DracoCompressor/assets/dunklowred.draco";

	auto result = Encode(input, output, options);
}