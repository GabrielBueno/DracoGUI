#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include <draco/compression/encode.h>
#include <draco/io/file_utils.h>
#include <draco/io/mesh_io.h>
#include <draco/io/stdio_file_reader.h>
#include <draco/io/stdio_file_writer.h>
#include <draco/io/file_reader_factory.h>
#include <draco/io/file_writer_factory.h>

namespace Compressor {
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
}

int main(void) {
	Compressor::Options options;

	draco::FileReaderFactory::RegisterReader(draco::StdioFileReader::Open);
	draco::FileWriterFactory::RegisterWriter(draco::StdioFileWriter::Open);

	options.pos_quantization_bits = 11;
	options.tex_coords_quantization_bits = 10;
	options.normals_quantization_bits = 8;
	options.generic_quantization_bits = 8;
	options.compression_level = 7;

	const char* input = "D:/Programacao/Projetos/3D/DracoCompressor/assets/dunklowred.obj";
	const char* output = "D:/Programacao/Projetos/3D/DracoCompressor/assets/dunklowred.draco";

	auto result = Compressor::Encode(input, output, options);

	return 0;
}