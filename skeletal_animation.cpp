#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// ================= Window / Camera =================
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

float camDistance = 5.0f;
float camHeight = 2.0f;
float camYawDeg = 0.0f;   
float camPitchDeg = 10.0f;  

bool  firstMouse = true;
float lastX = SCR_WIDTH * 0.5f;
float lastY = SCR_HEIGHT * 0.5f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ================ Character =================
struct Character {
    glm::vec3 position{ 0.0f, 0.4f, 0.0f };
    float yawDeg = 0.0f;    
    float speed = 0.0f;
    bool  jumpActive = false;
    float jumpT = 0.0f;
} player;

// ===== Jump Forward System =====
glm::vec3 jumpForwardVec(0.0f);
float jumpForwardSpeed = 0.0f;
const float JUMP_FORWARD_POWER = 4.0f;  
const float JUMP_FORWARD_DECAY = 2.5f;

// ================ Anim States =================
enum class AnimState { Idle, WalkForward, RunForward, WalkBackward, StrafeLeft, StrafeRight, Jump };
AnimState state = AnimState::Idle;

// ================ Crossfade =================
struct Crossfade {
    Animator* from = nullptr;
    Animator* to = nullptr;
    float t = 1.0f;
    float duration = 0.18f;
    bool  active() const { return t < 1.0f; }
    void start(Animator* a, Animator* b, float seconds) {
        from = a; to = b; t = 0.0f; duration = std::max(0.01f, seconds);
    }
    void update(float dt) { if (t < 1.0f) { t += dt / duration; if (t > 1.0f) t = 1.0f; } }
} xfade;

// ================ Jump Timing =================
const float JUMP_BLEND_IN = 0.12f;
const float JUMP_HOLD = 0.45f;
const float JUMP_BLEND_OUT = 0.18f;
const float JUMP_TOTAL = JUMP_BLEND_IN + JUMP_HOLD + JUMP_BLEND_OUT;

// ================ Helpers =================
glm::vec3 forwardFromYaw(float yawDeg) {
    float r = glm::radians(yawDeg);
    return glm::normalize(glm::vec3(std::sin(r), 0.0f, -std::cos(r)));
}
glm::vec3 rightFromYaw(float yawDeg) {
    float r = glm::radians(yawDeg);
    return glm::normalize(glm::vec3(std::cos(r), 0.0f, std::sin(r)));
}

static inline float angleDelta(float a, float b) {
    float d = b - a;
    while (d > 180.f) d -= 360.f;
    while (d < -180.f) d += 360.f;
    return d;
}

// ===== ปรับได้ตามโมเดลของคุณ =====
const float MODEL_YAW_OFFSET = 90.0f;   
const float FOOT_Y_OFFSET = -0.57f; 

// ================ Callbacks =================
void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;

    camYawDeg += xoffset * 0.15f;
    player.yawDeg += xoffset * 0.15f;
    camPitchDeg += yoffset * 0.12f;
    camPitchDeg = std::clamp(camPitchDeg, -30.0f, 45.0f);
    if (camYawDeg > 360.0f) camYawDeg -= 360.0f;
    if (camYawDeg < -360.0f) camYawDeg += 360.0f;
}

void scroll_callback(GLFWwindow*, double, double yoff) {
    camDistance = std::clamp(camDistance - (float)yoff, 3.0f, 10.0f);
}

// =================== Main ===================
int main() {
    // GL init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Animation", nullptr, nullptr);
    if (!window) { std::cout << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "Failed to initialize GLAD\n"; return -1; }
    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    Shader ourShader("anim_model.vs", "anim_model.fs");

    // -------- Map --------
    Model mapModel(FileSystem::getPath("resources/objects/map/rp_deadcity_no_vehicles.obj"));
    const float     MAP_SCALE = 1.0f;
    const glm::vec3 MAP_OFFSET = glm::vec3(0, 0, 0);

    // -------- Character + Animations --------
    Model     ourModel(FileSystem::getPath("resources/objects/uncle/Idle.dae"));
    Animation idleAnim(FileSystem::getPath("resources/objects/uncle/Idle.dae"), &ourModel);
    Animation walkAnim(FileSystem::getPath("resources/objects/uncle/Walking.dae"), &ourModel);
    Animation runAnim(FileSystem::getPath("resources/objects/uncle/Fast_Run.dae"), &ourModel);
    Animation backAnim(FileSystem::getPath("resources/objects/uncle/Walking_Backwards.dae"), &ourModel);
    Animation strafeLAnim(FileSystem::getPath("resources/objects/uncle/Left_Strafe_Walk.dae"), &ourModel);
    Animation strafeRAnim(FileSystem::getPath("resources/objects/uncle/Right_Strafe_Walk.dae"), &ourModel);
    Animation jumpAnim(FileSystem::getPath("resources/objects/uncle/Jumping.dae"), &ourModel);

    Animator animIdle(&idleAnim);
    Animator animWalk(&walkAnim);
    Animator animRun(&runAnim);
    Animator animBack(&backAnim);
    Animator animStrafeL(&strafeLAnim);
    Animator animStrafeR(&strafeRAnim);
    Animator animJump(&jumpAnim);

    Animator* current = &animIdle;
    Animator* target = &animIdle;

    // ================= Loop =================
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame; lastFrame = now;

        // Input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        bool keyW = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool keyS = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        bool keyA = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool keyD = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool keyShift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        bool keySpace = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

        // Jump (edge trigger)
        static bool prevSpace = false;
        if (!player.jumpActive && keySpace && !prevSpace) {
            player.jumpActive = true;
            player.jumpT = 0.0f;
            if (keyW) {
                jumpForwardVec = forwardFromYaw(camYawDeg);
                jumpForwardSpeed = JUMP_FORWARD_POWER;
            }
            else {
                jumpForwardVec = glm::vec3(0.0f);
                jumpForwardSpeed = 0.0f;
            }

            xfade.start(current, &animJump, JUMP_BLEND_IN);
            target = &animJump;
            state = AnimState::Jump;
        }
        prevSpace = keySpace;

        // Update animators
        animIdle.UpdateAnimation(deltaTime);
        animWalk.UpdateAnimation(deltaTime);
        animRun.UpdateAnimation(deltaTime);
        animBack.UpdateAnimation(deltaTime);
        animStrafeL.UpdateAnimation(deltaTime);
        animStrafeR.UpdateAnimation(deltaTime);
        animJump.UpdateAnimation(deltaTime);
        xfade.update(deltaTime);
        if (!xfade.active()) current = target;

        // Intent 
        float forwardIntent = (keyW ? 1.0f : 0.0f) + (keyS ? -1.0f : 0.0f);
        float strafeIntent = (keyD ? 1.0f : 0.0f) + (keyA ? -1.0f : 0.0f);

        AnimState desire = AnimState::Idle;
        Animator* want = &animIdle;
        if (forwardIntent > 0.0f) { desire = keyShift ? AnimState::RunForward : AnimState::WalkForward; want = keyShift ? &animRun : &animWalk; }
        else if (forwardIntent < 0.0f) { desire = AnimState::WalkBackward; want = &animBack; }
        else if (strafeIntent < 0.0f) { desire = AnimState::StrafeLeft;   want = &animStrafeL; }
        else if (strafeIntent > 0.0f) { desire = AnimState::StrafeRight;  want = &animStrafeR; }

        if (!player.jumpActive && want != target) { xfade.start(current, want, 0.10f); target = want; state = desire; }

        float maxWalk = 2.4f, maxRun = 4.8f;
        float targetSpeed = 0.0f;
        switch (state) {
        case AnimState::WalkForward:  targetSpeed = maxWalk; break;
        case AnimState::WalkBackward: targetSpeed = maxWalk * 0.8f; break;
        case AnimState::StrafeLeft:
        case AnimState::StrafeRight:  targetSpeed = maxWalk; break;
        case AnimState::RunForward:   targetSpeed = maxRun;  break;
        default: break;
        }
       
        player.speed += (targetSpeed - player.speed) * std::min(8.0f * deltaTime, 1.0f);

        glm::vec3 fwd = forwardFromYaw(camYawDeg);
        glm::vec3 rgt = rightFromYaw(camYawDeg);
        glm::vec3 moveVec = fwd * forwardIntent + rgt * strafeIntent;
        if (glm::length(moveVec) > 0.001f) moveVec = glm::normalize(moveVec);
        player.position += moveVec * player.speed * deltaTime;

        if (player.jumpActive && jumpForwardSpeed > 0.0f) {
            player.position += jumpForwardVec * jumpForwardSpeed * deltaTime;
            jumpForwardSpeed -= JUMP_FORWARD_DECAY * deltaTime;
            if (jumpForwardSpeed < 0.0f) jumpForwardSpeed = 0.0f;
        }

        player.position.y = 0.4f;

        float jumpY = 0.0f;
        if (player.jumpActive) {
            player.jumpT += deltaTime;
            float x = std::clamp(player.jumpT / JUMP_TOTAL, 0.0f, 1.0f);
            jumpY = 1.2f * (4.0f * x * (1.0f - x));
            if (player.jumpT >= JUMP_TOTAL) player.jumpActive = false;
        }

        // ======== Render ========
        glClearColor(0.05f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float camYawRad = glm::radians(camYawDeg);
        float camPitchRad = glm::radians(camPitchDeg);
        glm::vec3 camDir(
            std::sin(camYawRad) * std::cos(camPitchRad),
            std::sin(camPitchRad),
            -std::cos(camYawRad) * std::cos(camPitchRad)
        );
        glm::vec3 camPos = player.position - camDir * camDistance + glm::vec3(0, camHeight, 0);

        glm::mat4 view = glm::lookAt(
            camPos,
            player.position + glm::vec3(0, 1.0f, 0),
            glm::vec3(0, 1, 0)
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

        ourShader.use();
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);

        // --- MAP ---
        {
            ourShader.setMat4("finalBonesMatrices[0]", glm::mat4(1.0f)); // neutral
            glm::mat4 M(1.0f);
            M = glm::translate(M, MAP_OFFSET);
            M = glm::scale(M, glm::vec3(MAP_SCALE));
            ourShader.setMat4("model", M);
            mapModel.Draw(ourShader);
        }

        // --- PLAYER ---
        {
            const auto& A = (xfade.from ? xfade.from->GetFinalBoneMatrices() : current->GetFinalBoneMatrices());
            const auto& B = (xfade.to ? xfade.to->GetFinalBoneMatrices() : current->GetFinalBoneMatrices());
            size_t N = std::max(A.size(), B.size());
            float  t = std::clamp(xfade.t, 0.0f, 1.0f);
            for (size_t i = 0; i < N; ++i) {
                glm::mat4 a = (i < A.size() ? A[i] : glm::mat4(1.f));
                glm::mat4 b = (i < B.size() ? B[i] : glm::mat4(1.f));
                glm::mat4 m(0.0f);
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        m[r][c] = a[r][c] * (1.f - t) + b[r][c] * t;
                ourShader.setMat4("finalBonesMatrices[" + std::to_string((int)i) + "]", m);
            }

            glm::mat4 M(1.0f);
           
            M = glm::translate(M, player.position + glm::vec3(0.0f, jumpY + FOOT_Y_OFFSET, 0.0f));
            M = glm::rotate(M, glm::radians(player.yawDeg + 180.0f), glm::vec3(0, 1, 0));
            M = glm::scale(M, glm::vec3(0.5f));
            ourShader.setMat4("model", M);

            ourModel.Draw(ourShader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
