# Character-animation-control

## 🎬 Demo
> ![demo](<https://github.com/gygigies/Character-animation-control/blob/main/animation.mp4>)


## 🎮 Controls

| Input            | Action                                                                       |
|------------------|------------------------------------------------------------------------------|
| Mouse            | หมุนกล้อง                            |
| `W A S D`        | เดิน                                              |
| `Shift`          | Sprint                                                                       |
| `Space`          | กระโดด  |
| Scroll           | ปรับระยะกล้อง                                           |
| `ESC`            | ออกจากเกม                                                                    |

---

## ✨ What’s inside

- `main.cpp` — game loop, third-person camera, camera-relative movement, jump & forward-jump, skinned mesh rendering, simple state machine + crossfade, map draw  
- `shaders/anim_model.vs`, `shaders/anim_model.fs` — shader 

**Core techniques**
- **Always-face-camera**: ตัวละครหัน = `camYawDeg + 180°` (+ `MODEL_YAW_OFFSET` เพื่อชดเชยทิศหน้าโมเดล)
- **Camera-relative motion**: ใช้ `forwardFromYaw / rightFromYaw` จาก yaw กล้อง
- **Forward jump**: เมื่อ `Space + W` จะตั้ง `jumpForwardVec` และลดแรงด้วย `JUMP_FORWARD_DECAY`
- **Soft speed**: ไล่ความเร็วไปหา `targetSpeed` ตามสถานะแอนิเมชัน
- **Crossfade animations**: idle / walk / run / back / strafe / jump
- **Foot offset**: ปรับจม/ลอยพื้นด้วย `FOOT

## 🧾 Credits (3rd-party assets)

- Map from "Rp_dead_city_v1" (https://skfb.ly/pAAXZ) by Leafia dev. is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).
- Animation from Mixamo.com
