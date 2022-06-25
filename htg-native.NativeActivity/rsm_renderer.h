#ifndef __rosemary_project_renderer_h_
#define __rosemary_project_renderer_h_

#include "utils.hpp"

#include <GLES/gl.h>

#include <GLES3/gl32.h>
#include <vector>
#include <array>
#include <memory>

#include <map>
#include <unordered_map>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include "imgui/imgui_uielement.h"

namespace rsm {
    struct ImageAsset {
        int width;
        int height;
        GLuint imgid;
        // if returns false: use stbi_geterror!
        bool LoadFromAPK(const char* filename) {
            return ImGui::LoadTextureFromAPK(filename, &imgid, &width, &height);
        }
        bool LoadFromMemory(char* bytes, size_t len) {
            return ImGui::LoadTextureFromMemory((unsigned char*)bytes, len, &imgid, &width, &height);
        }
        operator ImTextureID() {
            return (void*)(intptr_t)imgid;
        }
    };
    class Shader
    {
    public:
        static inline const char* vertex_default_src = R"glsl(
            #version 150 core

            in vec2 position;

            void main()
            {
                gl_Position = vec4(position, 0.0, 1.0);
            }
        )glsl";
        static inline const char* fragment_default_src = R"glsl(
            #version 150 core

            out vec4 outColor;

            void main()
            {
                outColor = vec4(1.0, 1.0, 1.0, 1.0);
            }
        )glsl";
        std::string vertexSource = vertex_default_src;
        std::string fragmentSource = fragment_default_src;
        unsigned int ID;
        // constructor generates the shader on the fly
        // ------------------------------------------------------------------------
        Shader(const char* vertexShaderData, const char* fragmentShaderData)
        {
            // 2. compile shaders
            unsigned int vertex, fragment;
            // vertex shader
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vertexShaderData, NULL);
            glCompileShader(vertex);
            checkCompileErrors(vertex, "VERTEX");
            // fragment Shader
            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fragmentShaderData, NULL);
            glCompileShader(fragment);
            checkCompileErrors(fragment, "FRAGMENT");
            // shader Program
            ID = glCreateProgram();
            glAttachShader(ID, vertex);
            glAttachShader(ID, fragment);
            glLinkProgram(ID);
            checkCompileErrors(ID, "PROGRAM");
            // delete the shaders as they're linked into our program now and no longer necessery
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            vertexSource = vertexShaderData;
            fragmentSource = fragmentShaderData;
        }
        // activate the shader
        // ------------------------------------------------------------------------
        void use() const
        {
            glUseProgram(ID);
        }
        // utility uniform functions
        // ------------------------------------------------------------------------
        void setBool(const std::string& name, bool value) const
        {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
        }
        // ------------------------------------------------------------------------
        void setInt(const std::string& name, int value) const
        {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
        }
        // ------------------------------------------------------------------------
        void setFloat(const std::string& name, float value) const
        {
            glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
        }
        // ------------------------------------------------------------------------
        void setVec2(const std::string& name, const glm::vec2& value) const
        {
            glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
        void setVec2(const std::string& name, float x, float y) const
        {
            glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
        }
        // ------------------------------------------------------------------------
        void setVec3(const std::string& name, const glm::vec3& value) const
        {
            glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
        void setVec3(const std::string& name, float x, float y, float z) const
        {
            glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
        }
        // ------------------------------------------------------------------------
        void setVec4(const std::string& name, const glm::vec4& value) const
        {
            glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
        void setVec4(const std::string& name, float x, float y, float z, float w) const
        {
            glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
        }
        // ------------------------------------------------------------------------
        void setMat2(const std::string& name, const glm::mat2& mat) const
        {
            glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        // ------------------------------------------------------------------------
        void setMat3(const std::string& name, const glm::mat3& mat) const
        {
            glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        // ------------------------------------------------------------------------
        void setMat4(const std::string& name, const glm::mat4& mat) const
        {
            glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }

    private:
        // utility function for checking shader compilation/linking errors.
        // ------------------------------------------------------------------------
        void checkCompileErrors(GLuint shader, std::string type)
        {
            GLint success;
            GLchar infoLog[1024];
            if (type != "PROGRAM")
            {
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                    __android_log_print(ANDROID_LOG_ERROR, "rsm:renderer", "Unable to compile shader of type: %s, %s", type.c_str(), infoLog);
                }
            }
            else
            {
                glGetProgramiv(shader, GL_LINK_STATUS, &success);
                if (!success)
                {
                    glGetProgramInfoLog(shader, 1024, NULL, infoLog);

                    __android_log_print(ANDROID_LOG_ERROR, "rsm:renderer", "Unable to link shader of type: %s, %s", type.c_str(), infoLog);
                }
            }
        }
    };
    struct Mesh {
    private:
        Shader* _shader;
        bool buffersGenned;
    public:
        float vertices[0] = {};
        size_t vertices_length;
        unsigned int VBO, VAO;
        unsigned int TexID;
        void UseShaderData(const char* vshaderdata, const char* fshaderdata) {
            if (_shader) { delete(_shader); }
            _shader = new Shader(vshaderdata, fshaderdata);
        }
        Shader* getShader() { return _shader; }
        void GenTextureFromFile(unsigned char* file_data, size_t len) {
            glGenTextures(1, &TexID);
            glBindTexture(GL_TEXTURE_2D, TexID);
            // set the texture wrapping parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            // set texture filtering parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // load image, create texture and generate mipmaps
            int width, height, nrChannels;
            stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
            unsigned char* data = stbi_load_from_memory(file_data, len, &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
            }
            else
            {
                __android_log_print(ANDROID_LOG_ERROR, "rsm:renderer", "Unable to load image data.");
            }
            stbi_image_free(data);
        }
        void GenBuffers() {
            if (!_shader) {
                // create default shader
                UseShaderData(Shader::vertex_default_src, Shader::fragment_default_src);
            }
            buffersGenned = true;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices_length, vertices, GL_STATIC_DRAW);
        }
        void Render() {
            if (!buffersGenned) { GenBuffers(); }
            if (_shader) { _shader->use(); }
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 1);
        }
    };
	struct Cube : public Mesh{
        // 6 per face, 3 per vertex, 6 faces + 2 UVs per face
        float vertices[6 * 5 * 6] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
        };
        size_t vertices_length = 180;
	};
	struct Camera {
	private:
		glm::vec3 _right;
		glm::vec3 _up;
		glm::vec3 _rd;
		glm::vec3 _rot;
		glm::vec3 _front;
	public:
		static inline glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
		static inline glm::vec3 front= glm::vec3(0.0f, 0.0f, -1.0f);
		static inline glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		float yaw = -90.0f;
		float pitch = 0.0f;
		float fov = 45.0f;
		float near = 0.01f;
		float far = FLT_MAX;
		static inline double width = 1080;
		static inline double height = 720;
		glm::mat4 getProjection() {return glm::perspective(glm::radians(fov), (float)width / (float)height, near, far);
		}
	};

};


#endif