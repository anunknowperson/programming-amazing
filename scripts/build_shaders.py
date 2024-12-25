import os
import subprocess
import argparse

def main():
    parser = argparse.ArgumentParser(description='Compile GLSL shaders to SPIR-V')
    parser.add_argument('source_dir', help='Directory containing source shader files')
    parser.add_argument('output_dir', help='Directory for compiled shader output')
    args = parser.parse_args()

    # Ensure output directory exists
    os.makedirs(args.output_dir, exist_ok=True)

    # Select appropriate glslc executable based on platform
    glslc_executable = "glslc.exe" if os.name == 'nt' else "glslc"

    print(f"Compiling shaders from {args.source_dir} to {args.output_dir}")

    for filename in os.listdir(args.source_dir):
        if filename.endswith((".vert", ".frag", ".comp")):
            shader_file_path = os.path.join(args.source_dir, filename)
            output_file = os.path.join(args.output_dir, filename + ".spv")
            
            command = [glslc_executable, shader_file_path, "-o", output_file]
            try:
                subprocess.run(command, check=True)
                print(f"Compiled {filename} to {output_file}")
            except subprocess.CalledProcessError as e:
                print(f"Failed to compile {filename}: {e}")

if __name__ == "__main__":
    main()