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

#include "ndk-helper/NDKHelper.h"
#include "ani.h"

namespace rsm {
    static ndk_helper::DoubletapDetector doubletap_detector_;
    static ndk_helper::PinchDetector pinch_detector_;
    static ndk_helper::DragDetector drag_detector_;
    static ndk_helper::PerfMonitor monitor_;
    static ndk_helper::TapCamera tap_camera_;
    static bool DoubleTapped;
    static float DoubleTapSeconds;
    enum {
        rsm_DoubleTapped_First = 4,
        rsm_DoubleTapped_End = 5,
        rsm_DoubleTappedActive = 6,
    };
    static glm::vec2 PinchPositionBegin, PinchPositionEnd, DragPosition, DoubleTapPosition;
    static glm::vec2 m_PinchPositionBeginL, m_PinchPositionEndL, m_DragPositionL, m_DoubleTapPositionL;
    static glm::vec2 getPinch1Delta() {
        return m_PinchPositionBeginL - PinchPositionBegin;
    }
    static glm::vec2 getPinch2Delta() {
        return m_PinchPositionEndL - PinchPositionEnd;
    }
    static glm::vec2 getDragDelta() {
        ImVec2 tmp = ImGui::GetIO().MouseDelta;
        return {tmp.x, tmp.y};
    }
    namespace Renderer {
        static void TransformPosition(ndk_helper::Vec2& vec);
        struct Shader {
            // vertex shader source. modify and then call compile()!
            std::string vtxsrc, vtxdefault = R"(
                #version 300 es
                layout (location = 0) in vec3 aPos;

                uniform mat4 matrix;
                void main()
                {
	                gl_Position = matrix * vec4(aPos, 1.0f);
                }
            )";
            // read vtxsrc comment
            std::string frgsrc, frgdefault = R"(
                #version 300 es
                out vec4 FragColor;

                void main()
                {
	                FragColor = vec4( 1.0, 0, 0, 0.2f );
                }
            )";
            GLuint shader_id = 0;
            
            void compile() {
                const char* vShaderCode = vtxsrc.c_str();
                const char* fShaderCode = frgsrc.c_str();
                unsigned int vertex, fragment;
                shader_id = glCreateProgram();

                vertex = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(vertex, 1, &vShaderCode, NULL);
                glCompileShader(vertex);
                if (checkCompileErrors(vertex, "VERTEX")) { return; };
                // fragment Shader
                fragment = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(fragment, 1, &fShaderCode, NULL);
                glCompileShader(fragment);
                if (checkCompileErrors(fragment, "FRAGMENT")) { return; };
                
                glAttachShader(shader_id, vertex);
                glAttachShader(shader_id, fragment);
                glLinkProgram(shader_id);
                if (checkCompileErrors(shader_id, "PROGRAM")) { return; };
                // delete the shaders as they're linked into our program now and no longer necessery
                glDeleteShader(vertex);
                glDeleteShader(fragment);
            }
            Shader(std::string vertex_shader_source = "", std::string fragment_shader_source = "") {
                if (vertex_shader_source.empty()) {
                    vertex_shader_source = vtxdefault;
                }
                if (fragment_shader_source.empty()) {
                    fragment_shader_source = frgdefault;
                }
                vtxsrc = vertex_shader_source;
                frgsrc = fragment_shader_source;
            }
            void use() const {
                if (!glIsProgram(shader_id)) { return; }
                glUseProgram(shader_id);
            }
            void erase() const {
                if (!glIsProgram(shader_id)) { return; }
                glDeleteProgram(shader_id);
            }
            // utility uniform functions
            // ------------------------------------------------------------------------
            void setBool(const std::string& name, bool value) const
            {
                glUniform1i(glGetUniformLocation(shader_id, name.c_str()), (int)value);
            }
            // ------------------------------------------------------------------------
            void setInt(const std::string& name, int value) const
            {

                glUniform1i(glGetUniformLocation(shader_id, name.c_str()), value);
            }
            // ------------------------------------------------------------------------
            void setFloat(const std::string& name, float value) const
            {
                glUniform1f(glGetUniformLocation(shader_id, name.c_str()), value);
            }
            // ------------------------------------------------------------------------
            void setVec2(const std::string& name, const glm::vec2& value) const
            {
                glUniform2fv(glGetUniformLocation(shader_id, name.c_str()), 1, &value[0]);
            }
            void setVec2(const std::string& name, float x, float y) const
            {
                glUniform2f(glGetUniformLocation(shader_id, name.c_str()), x, y);
            }
            // ------------------------------------------------------------------------
            void setVec3(const std::string& name, const glm::vec3& value) const
            {
                glUniform3fv(glGetUniformLocation(shader_id, name.c_str()), 1, &value[0]);
            }
            void setVec3(const std::string& name, float x, float y, float z) const
            {
                glUniform3f(glGetUniformLocation(shader_id, name.c_str()), x, y, z);
            }
            // ------------------------------------------------------------------------
            void setVec4(const std::string& name, const glm::vec4& value) const
            {
                glUniform4fv(glGetUniformLocation(shader_id, name.c_str()), 1, &value[0]);
            }
            void setVec4(const std::string& name, float x, float y, float z, float w) const
            {
                glUniform4f(glGetUniformLocation(shader_id, name.c_str()), x, y, z, w);
            }
            // ------------------------------------------------------------------------
            void setMat2(const std::string& name, const glm::mat2& mat) const
            {
                glUniformMatrix2fv(glGetUniformLocation(shader_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
            }
            // ------------------------------------------------------------------------
            void setMat3(const std::string& name, const glm::mat3& mat) const
            {
                glUniformMatrix3fv(glGetUniformLocation(shader_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
            }
            // ------------------------------------------------------------------------
            void setMat4(const std::string& name, const glm::mat4& mat) const
            {
                glUniformMatrix4fv(glGetUniformLocation(shader_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
            }
            // utility function for checking shader compilation/linking errors.
            // ------------------------------------------------------------------------
            bool checkCompileErrors(GLuint shader, std::string type)
            {
                GLint success;
                GLchar infoLog[1024];
                if (type != "PROGRAM")
                {
                    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                    if (!success)
                    {
                        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                        cwError::serrof("\\FCC0C0]SHADER_COMPILATION_ERROR of type: %s\n%s", type.c_str(), infoLog);
                        return 1;
                    }
                }
                else
                {
                    glGetProgramiv(shader, GL_LINK_STATUS, &success);
                    if (!success)
                    {
                        glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                        cwError::serrof("\\FCC0C0]PROGRAM_LINKING_ERROR of type: %s\n%s", type.c_str(), infoLog);
                        return 1;
                    }
                }
                return 0;
            }
        };
        struct Camera {
        private:
            glm::vec3 _right;
            glm::vec3 _up;
            glm::vec3 _rd;
            glm::vec3 _rot;
            glm::vec3 _front;
        public:
            float yaw = -90.0f;
            float pitch = 0.0f;
            float fov = 75.0f;
            float near = 0.01f;
            float far = 1000;
            static inline int width = 1080;
            static inline int height = 720;
            glm::vec2 getWindowClip() {
                height = ANativeWindow_getHeight(ANIEnv::app_g->window);
                width = ANativeWindow_getWidth(ANIEnv::app_g->window);
                return { width, height };
            }
            std::vector<char> m_pixbuf;
            bool pixbuf_dirty_alpha = false;
            void GLRenderBuffer(bool alpha = false) {
                getWindowClip();
                m_pixbuf.reserve((alpha) ? 4 : 3 * width * height);
                m_pixbuf.clear();
                glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, &m_pixbuf.at(0));
                pixbuf_dirty_alpha = alpha;
            }
            std::vector<char>* GetPixelBuffer(bool * is_alpha = 0) {
                if (is_alpha) {
                    *is_alpha = pixbuf_dirty_alpha;
                }
                return &m_pixbuf;
            }
            // note: this is INVERTED! Use setPosition() and getPosition() respectively.
            glm::vec3 m_lookat_pos = glm::vec3(0.0f, 0.0f, 0.0f);
            float lookat_distance;
            glm::vec3 m_position = glm::vec3{0.0f, 0.0f, -12.0f};
            // in degrees (0-360)
            glm::vec3 m_rotation = glm::vec3(0.0f, 0.0f, 180.0f);
            glm::mat4 m_view = glm::mat4(1.0f);
            glm::vec3 setPosition(glm::vec3 pos) {
                return -m_position = -pos;
            }
            glm::vec3 getPosition() {
                return -m_position;
            }
            glm::vec3 getRotation() {
                return m_rotation;
            }
            glm::vec3 setRotation(glm::vec3 rot) {
                return m_rotation = rot;
            }
            // Generic transform type.
            struct transform_t {
                float* m_x; float* m_y; float* m_z;
                transform_t(float* xp, float* yp, float *zp) {
                    m_x = xp; m_y = yp; m_z = zp;
                }
                void x(float _x) { *m_x += _x; };
                void y(float _y) { *m_y -= _y; };
                void z(float _z) { *m_z += _z; };
                void xy(float _x, float _y) { x(_x); y(_y); }
                void xyz(float _x, float _y, float _z) { x(_x); y(_y); z(_z); }
            };
            transform_t transform_position{&m_position.x, &m_position.y, &m_position.z};
            transform_t transform_rotation{&m_rotation.x, &m_rotation.y, &m_rotation.z };
            glm::mat4 GetFullPerspectiveMatrix() {
                glm::mat4 view = glm::mat4(1.0f);
                getWindowClip();
                glm::mat4 projection = glm::perspective(glm::radians(fov), ( (float)width / (float)height ), near, far);
                glm::mat4 model = glm::mat4(1.0f);
                view = glm::translate(view, m_position);
                // x axis
                model = glm::rotate(model, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                // y axis
                model = glm::rotate(model, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                // z axis
                model = glm::rotate(model, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                return projection * view * model;
            }
            // [0] : projection
            // [1] : view
            // [2] : model
            std::array<glm::mat4, 3> GetPartialPerspectiveMatrix() {
                glm::mat4 view = glm::mat4(1.0f);
                getWindowClip();
                glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, near, far);
                glm::mat4 model = glm::mat4(1.0f);
                view = glm::translate(view, m_position);
                // x axis
                model = glm::rotate(model, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                // y axis
                model = glm::rotate(model, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                // z axis
                model = glm::rotate(model, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                return { projection, view, model };
            }
        };
    };
    struct ImageAsset {
        int width;
        int height;
        GLuint imgid;
        static inline std::function<void(const char*)> on_error = [](const char*) {return; };
        // if returns false: use stbi_geterror!
        bool LoadFromAPK(const char* filename) {
            bool state = ImGui::LoadTextureFromAPK(filename, &imgid, &width, &height);
            if (!state) {
                on_error(alib_strfmts("Unable to load texture from APK: %s", stbi_failure_reason()).c_str());
            }
            return state;
        }
        bool LoadFromMemory(char* bytes, size_t len) {
            bool state = ImGui::LoadTextureFromMemory((unsigned char*)bytes, len, &imgid, &width, &height);
            if (!state) {
                on_error(alib_strfmts("Unable to load texture from memory: %s", stbi_failure_reason()).c_str());
            }
            return state;
        }
        operator ImTextureID() {
            return (void*)(intptr_t)imgid;
        }

    };
};


#endif