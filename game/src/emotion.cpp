#include "raylib.h"
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <tuple>

Vector2 CalculateQuadraticBezierPoint(Vector2 start, Vector2 control, Vector2 end, float t) {
    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    return {
        (uu * start.x) + (2 * u * t * control.x) + (tt * end.x),
        (uu * start.y) + (2 * u * t * control.y) + (tt * end.y)
    };
}

typedef enum {
    EMOTION_HAPPY,
    EMOTION_SAD,
    EMOTION_ANGRY,
    EMOTION_BORED,
    EMOTION_NOT_IMPRESSED,
    EMOTION_EVIL_IDEA,
    EMOTION_FLIRTY,
    EMOTION_AWWW,
    EMOTION_WTF
} Emotion;

std::atomic<Emotion> targetEmotion(EMOTION_HAPPY);
Emotion displayedEmotion = EMOTION_HAPPY;
float transitionTime = 0.0f;
const float transitionDuration = 2.0f; // Make transition last longer

void socketListener() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        int valread = read(new_socket, buffer, 1024);
        if (valread > 0) {
            std::string emotion(buffer, valread);
            Emotion newEmotion = targetEmotion.load();
            if (emotion == "happy") newEmotion = EMOTION_HAPPY;
            else if (emotion == "sad") newEmotion = EMOTION_SAD;
            else if (emotion == "angry") newEmotion = EMOTION_ANGRY;
            else if (emotion == "bored") newEmotion = EMOTION_BORED;
            else if (emotion == "not_impressed") newEmotion = EMOTION_NOT_IMPRESSED;
            else if (emotion == "evil_idea") newEmotion = EMOTION_EVIL_IDEA;
            else if (emotion == "flirty") newEmotion = EMOTION_FLIRTY;
            else if (emotion == "aww") newEmotion = EMOTION_AWWW;
            else if (emotion == "wtf") newEmotion = EMOTION_WTF;

            if (newEmotion != targetEmotion.load()) {
                targetEmotion = newEmotion;
                transitionTime = 0.0001f;
            }
        }

        close(new_socket);
    }
}

Vector2 LerpVec2(Vector2 a, Vector2 b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

std::vector<std::pair<Vector2, Vector2>> getEyebrows(Emotion e, int faceX, int faceY) {
    switch (e) {
        case EMOTION_ANGRY:
        case EMOTION_EVIL_IDEA:
            return {{{faceX - 100, faceY - 100}, {faceX - 40, faceY - 80}}, {{faceX + 40, faceY - 80}, {faceX + 100, faceY - 100}}};
        case EMOTION_BORED:
        case EMOTION_NOT_IMPRESSED:
            return {{{faceX - 100, faceY - 90}, {faceX - 40, faceY - 90}}, {{faceX + 40, faceY - 90}, {faceX + 100, faceY - 90}}};
        case EMOTION_FLIRTY:
            return {{{faceX - 100, faceY - 110}, {faceX - 40, faceY - 110}}, {{faceX + 40, faceY - 130}, {faceX + 100, faceY - 110}}};
        case EMOTION_AWWW:
            return {{{faceX - 110, faceY - 90}, {faceX - 50, faceY - 100}}, {{faceX + 50, faceY - 100}, {faceX + 110, faceY - 90}}};
        case EMOTION_WTF:
            return {{{faceX - 80, faceY - 110}, {faceX - 30, faceY - 110}}, {{faceX + 40, faceY - 130}, {faceX + 100, faceY - 100}}};
        default:
            return {};
    }
}

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Facial Expression Display");
    std::thread socketThread(socketListener);
    socketThread.detach();
    std::cout << "Server started. Listening on port 8080..." << std::endl;

    float blinkTimer = 0.0f, blinkDuration = 0.5f, blinkInterval = 3.0f;
    bool isBlinking = false;
    float mouthAnimationTimer = 0.0f, mouthAnimationSpeed = 4.0f;
    const float faceThickness = 5.0f, eyeRadius = 10.0f;
    SetTargetFPS(60);

    Vector2 prevStart, prevCtrl, prevEnd;
    Vector2 nextStart, nextCtrl, nextEnd;

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();
        blinkTimer += delta;
        if (blinkTimer >= blinkInterval) {
            isBlinking = true;
            blinkTimer = 0.0f;
        }
        if (isBlinking && blinkTimer >= blinkDuration) isBlinking = false;

        mouthAnimationTimer += delta;
        float mouthOffset = sinf(mouthAnimationTimer * mouthAnimationSpeed) * 5.0f;

        Emotion current = displayedEmotion;
        Emotion target = targetEmotion.load();

        if (current != target && transitionTime == 0.0f) {
            transitionTime = 0.0001f;
        }

        if (transitionTime > 0.0f) {
            transitionTime += delta;
            if (transitionTime >= transitionDuration) {
                transitionTime = 0.0f;
                displayedEmotion = targetEmotion.load();
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        int faceX = screenWidth / 2;
        int faceY = screenHeight / 2 - 100;

        if (isBlinking) {
            DrawLineEx({faceX - 80, faceY - 50}, {faceX - 40, faceY - 50}, faceThickness, BLACK);
            DrawLineEx({faceX + 40, faceY - 50}, {faceX + 80, faceY - 50}, faceThickness, BLACK);
        } else {
            DrawCircle(faceX - 60, faceY - 50, eyeRadius, BLACK);
            DrawCircle(faceX + 60, faceY - 50, eyeRadius, BLACK);
        }

        auto getMouth = [&](Emotion e) -> std::tuple<Vector2, Vector2, Vector2> {
            switch (e) {
                case EMOTION_HAPPY:
                    return {{faceX - 80, faceY + 50}, {faceX, faceY + 100}, {faceX + 80, faceY + 50}};
                case EMOTION_SAD:
                    return {{faceX - 80, faceY + 100}, {faceX, faceY + 50}, {faceX + 80, faceY + 100}};
                case EMOTION_ANGRY:
                    return {{faceX - 80, faceY + 100}, {faceX, faceY + 50}, {faceX + 80, faceY + 100}};
                case EMOTION_EVIL_IDEA:
                    return {{faceX - 90, faceY + 40}, {faceX, faceY + 110}, {faceX + 90, faceY + 40}};
                case EMOTION_BORED:
                    return {{faceX - 80, faceY + 75}, {faceX, faceY + 75}, {faceX + 80, faceY + 75}};
                case EMOTION_NOT_IMPRESSED:
                    return {{faceX - 80, faceY + 90}, {faceX, faceY + 60}, {faceX + 80, faceY + 90}};
                case EMOTION_FLIRTY:
                    return {{faceX - 80, faceY + 60}, {faceX, faceY + 85}, {faceX + 80, faceY + 60}};
                case EMOTION_AWWW:
                    return {{faceX - 90, faceY + 65}, {faceX, faceY + 110}, {faceX + 90, faceY + 65}};
                case EMOTION_WTF:
                    return {{faceX - 70, faceY + 90}, {faceX, faceY + 30}, {faceX + 70, faceY + 90}};
            }
            return {{faceX - 80, faceY + 75}, {faceX, faceY + 75}, {faceX + 80, faceY + 75}};
        };

        std::tie(prevStart, prevCtrl, prevEnd) = getMouth(displayedEmotion);
        std::tie(nextStart, nextCtrl, nextEnd) = getMouth(targetEmotion);

        float t = transitionDuration > 0 ? (transitionTime / transitionDuration) : 1.0f;
        if (t > 1.0f) t = 1.0f;

        Vector2 mouthStart = LerpVec2(prevStart, nextStart, t);
        Vector2 mouthControl = LerpVec2(prevCtrl, nextCtrl, t);
        Vector2 mouthEnd = LerpVec2(prevEnd, nextEnd, t);

        for (float tt = 0; tt <= 1; tt += 0.01f) {
            Vector2 point = CalculateQuadraticBezierPoint(mouthStart, mouthControl, mouthEnd, tt);
            DrawCircleV(point, faceThickness, BLACK);
        }

        auto prevBrows = getEyebrows(displayedEmotion, faceX, faceY);
        auto nextBrows = getEyebrows(targetEmotion, faceX, faceY);

        for (size_t i = 0; i < std::min(prevBrows.size(), nextBrows.size()); ++i) {
            Vector2 browStart = LerpVec2(prevBrows[i].first, nextBrows[i].first, t);
            Vector2 browEnd = LerpVec2(prevBrows[i].second, nextBrows[i].second, t);
            DrawLineEx(browStart, browEnd, faceThickness, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
