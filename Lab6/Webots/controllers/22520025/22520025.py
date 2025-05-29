from controller import Robot
import numpy as np
import skfuzzy as fuzz
import skfuzzy.control as ctrl
import sys # Import sys để thoát chương trình khi cần

# --- Cấu hình chung ---
# Tốc độ tối đa của động cơ e-puck (rad/s)
MAX_MOTOR_SPEED = 4

# Thời gian bước mô phỏng: 
# robot.getBasicTimeStep() thường là 32ms. Nhân với 4 để có bước 128ms, giúp mượt mà hơn.
TIME_STEP_FACTOR = 4

# --- Cấu hình cảm biến ---
DISTANCE_SENSORS_NAMES = ['ps0', 'ps1', 'ps2', 'ps3', 'ps4', 'ps5', 'ps6', 'ps7']
GROUND_SENSORS_NAMES   = ['gs0', 'gs1', 'gs2']
LEDS_NAMES             = [f'led{i}' for i in range(10)]

# Giá trị đọc tối đa của cảm biến khoảng cách e-puck
SENSOR_MAX_RAW_VALUE = 4095.0 
CLIFF_THRESHOLD      = 500.0 # Ngưỡng phát hiện vách đá cho cảm biến mặt đất

# --- Cấu hình Braitenberg (chủ yếu để tính toán hướng rẽ ban đầu) ---
# Tốc độ cơ bản cho mỗi bánh xe khi không có vật cản (trước khi điều chỉnh fuzzy)
# Đã giảm để robot chậm hơn
BASE_SPEED_OFFSET  = 0.3 * MAX_MOTOR_SPEED 
OFFSETS            = [BASE_SPEED_OFFSET, BASE_SPEED_OFFSET]

# Ma trận trọng số Braitenberg
# Các giá trị này ảnh hưởng đến hướng rẽ của robot dựa trên cảm biến
WEIGHTS            = [
    (-1.3, -1.0),  # ps0 (trước-trái)
    (-1.3, -1.0),  # ps1 (trái-trước)
    (-0.5,  0.5),  # ps2 (trái)
    ( 0.0,  0.0),  # ps3 (sau-trái)
    ( 0.0,  0.0),  # ps4 (sau-phải)
    ( 0.5, -0.5),  # ps5 (phải)
    (-0.75, 0.0),  # ps6 (phải-trước)
    (-0.75, 0.0)   # ps7 (trước-phải)
]

# --- Cấu hình Fuzzy Logic ---
# Khoảng giá trị chuẩn hóa cho cảm biến [0, 1]
DISTANCE_UNIVERSE = np.linspace(0, 1, 101)
# Khoảng giá trị cho đầu ra tốc độ [0, 1] (0: dừng, 1: tốc độ tối đa)
SPEED_UNIVERSE  = np.linspace(0, 1, 101) 

# Antecedents (biến đầu vào)
front_dist = ctrl.Antecedent(DISTANCE_UNIVERSE, 'front_distance')
left_dist  = ctrl.Antecedent(DISTANCE_UNIVERSE, 'left_distance')
right_dist = ctrl.Antecedent(DISTANCE_UNIVERSE, 'right_distance')

# Consequent (biến đầu ra)
speed_factor = ctrl.Consequent(SPEED_UNIVERSE, 'speed_factor')

# Định nghĩa hàm thành viên cho khoảng cách (chuẩn hóa)
front_dist['very_near'] = fuzz.trapmf(DISTANCE_UNIVERSE, [0.0, 0.0, 0.04, 0.15])
front_dist['near']      = fuzz.trimf(DISTANCE_UNIVERSE, [0.10, 0.25, 0.45])
front_dist['medium']    = fuzz.trimf(DISTANCE_UNIVERSE, [0.40, 0.60, 0.80])
front_dist['far']       = fuzz.trapmf(DISTANCE_UNIVERSE, [0.75, 0.90, 1.0, 1.0])

left_dist['very_near'] = fuzz.trapmf(DISTANCE_UNIVERSE, [0.0, 0.0, 0.04, 0.15])
left_dist['near']      = fuzz.trimf(DISTANCE_UNIVERSE, [0.10, 0.25, 0.45])
left_dist['medium']    = fuzz.trimf(DISTANCE_UNIVERSE, [0.40, 0.60, 0.80])
left_dist['far']       = fuzz.trapmf(DISTANCE_UNIVERSE, [0.75, 0.90, 1.0, 1.0])

right_dist['very_near'] = fuzz.trapmf(DISTANCE_UNIVERSE, [0.0, 0.0, 0.04, 0.15])
right_dist['near']      = fuzz.trimf(DISTANCE_UNIVERSE, [0.10, 0.25, 0.45])
right_dist['medium']    = fuzz.trimf(DISTANCE_UNIVERSE, [0.40, 0.60, 0.80])
right_dist['far']       = fuzz.trapmf(DISTANCE_UNIVERSE, [0.75, 0.90, 1.0, 1.0])

# Định nghĩa hàm thành viên cho đầu ra tốc độ
speed_factor['stop']   = fuzz.trimf(SPEED_UNIVERSE, [0.0, 0.0, 0.05])  # Dừng hẳn
speed_factor['very_slow'] = fuzz.trimf(SPEED_UNIVERSE, [0.03, 0.1, 0.2]) # Rất chậm, nhưng vẫn di chuyển
speed_factor['slow']   = fuzz.trimf(SPEED_UNIVERSE, [0.15, 0.3, 0.5])  # Chậm
speed_factor['medium'] = fuzz.trimf(SPEED_UNIVERSE, [0.4, 0.6, 0.8])  # Trung bình
speed_factor['fast']   = fuzz.trimf(SPEED_UNIVERSE, [0.7, 1.0, 1.0])   # Nhanh

# --- Luật Fuzzy ---
rules = [
    # 1. Ưu tiên cao nhất: Vật cản cực gần ở bất kỳ phía nào -> Dừng
    ctrl.Rule(front_dist['very_near'] | left_dist['very_near'] | right_dist['very_near'], speed_factor['stop']),

    # 2. Vật cản gần phía trước -> Chậm lại
    ctrl.Rule(front_dist['near'], speed_factor['slow']),

    # 3. Vật cản vừa phải ở phía trước -> Tốc độ trung bình
    ctrl.Rule(front_dist['medium'], speed_factor['medium']),

    # 4. Không có vật cản phía trước (xa) -> Tốc độ nhanh
    ctrl.Rule(front_dist['far'], speed_factor['fast']),

    # 5. Vật cản gần hai bên (nhưng phía trước không quá gần) -> Chậm lại một chút
    ctrl.Rule((left_dist['near'] | right_dist['near']) & ~front_dist['very_near'], speed_factor['slow']),

    # 6. Vật cản vừa phải hai bên -> Tốc độ trung bình
    ctrl.Rule((left_dist['medium'] | right_dist['medium']), speed_factor['medium']),

    # 7. Không có vật cản hai bên (xa) -> Tốc độ nhanh
    ctrl.Rule((left_dist['far'] & right_dist['far']) & front_dist['far'], speed_factor['fast'])
]

speed_control_system = ctrl.ControlSystem(rules)
speed_simulation     = ctrl.ControlSystemSimulation(speed_control_system)

# --- Cấu hình xử lý kẹt (Stuck Recovery) ---
STUCK_THRESHOLD_COUNT = 15 # Số lần liên tiếp cảm biến không đổi để coi là bị kẹt. Giảm ngưỡng này để robot phản ứng nhanh hơn.
BACKUP_DURATION_MS_RECOVERY = 600 # Thời gian lùi khi bị kẹt (ms)
TURN_DURATION_MS_RECOVERY = 800 # Thời gian rẽ khi bị kẹt (ms)
REVERSE_SPEED_RECOVERY = -0.7 # Tốc độ lùi khi phục hồi
TURN_SPEED_RECOVERY = 0.7  # Tốc độ rẽ khi phục hồi

# --- Biến cho cơ chế xử lý kẹt ---
stuck_counter = 0
last_sensor_reading = None


# --- Hàm Tiện ích ---

def set_wheel_speeds(left_m, right_m, vl, vr):
    """Đặt tốc độ cho bánh xe, đảm bảo nằm trong giới hạn MAX_MOTOR_SPEED."""
    vl_clipped = np.clip(vl, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED)
    vr_clipped = np.clip(vr, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED)
    left_m.setVelocity(vl_clipped)
    right_m.setVelocity(vr_clipped)

def get_normalized_distance_readings(dist_sensors_dict):
    """
    Đọc giá trị từ cảm biến khoảng cách và chuẩn hóa về khoảng [0, 1].
    Sử dụng trung bình các giá trị của các cảm biến tương ứng để giảm nhiễu.
    """
    front_val = (dist_sensors_dict['ps0'].getValue() + dist_sensors_dict['ps7'].getValue()) / 2.0
    front_val = np.clip(front_val, 0, SENSOR_MAX_RAW_VALUE) / SENSOR_MAX_RAW_VALUE

    left_val = (dist_sensors_dict['ps1'].getValue() + dist_sensors_dict['ps2'].getValue()) / 2.0
    left_val = np.clip(left_val, 0, SENSOR_MAX_RAW_VALUE) / SENSOR_MAX_RAW_VALUE

    right_val = (dist_sensors_dict['ps5'].getValue() + dist_sensors_dict['ps6'].getValue()) / 2.0
    right_val = np.clip(right_val, 0, SENSOR_MAX_RAW_VALUE) / SENSOR_MAX_RAW_VALUE
    
    return front_val, left_val, right_val

def get_ground_sensor_values(ground_sensors_dict):
    """Đọc giá trị cảm biến mặt đất (nếu tồn tại)."""
    vals = {}
    for name, gs in ground_sensors_dict.items():
        vals[name] = gs.getValue()
    return vals

def check_cliff_detection(ground_vals_dict):
    """Kiểm tra xem có vách đá nào được phát hiện không."""
    for name, val in ground_vals_dict.items():
        # Giá trị cảm biến mặt đất thấp (ví dụ < 500) thường báo hiệu vách đá
        if val is not None and val < CLIFF_THRESHOLD:
            return True
    return False

def calculate_braitenberg_turn(dist_vals_raw):
    """
    Tính toán lệnh rẽ dựa trên thuật toán Braitenberg.
    Trả về một "turn_command" từ -1 (rẽ gắt trái) đến 1 (rẽ gắt phải).
    """
    # Tạo một bản sao danh sách các giá trị thô để tránh ảnh hưởng đến list gốc
    # Mặc dù trong Braitenberg, chúng ta sử dụng giá trị thô, 
    # việc chuẩn hóa tạm thời giúp các trọng số có ý nghĩa hơn.
    normalized_dist_vals = [np.clip(val, 0, SENSOR_MAX_RAW_VALUE) / SENSOR_MAX_RAW_VALUE for val in dist_vals_raw]

    speeds_influence = [0.0, 0.0]
    for i in (0, 1): # 0 cho bánh trái, 1 cho bánh phải
        total_influence = 0.0
        for j, val_norm in enumerate(normalized_dist_vals):
            w = WEIGHTS[j][i]
            total_influence += val_norm * w
        
        speeds_influence[i] = OFFSETS[i] + total_influence * MAX_MOTOR_SPEED
    
    # Tính toán tỉ lệ rẽ: (tốc độ mong muốn phải - tốc độ mong muốn trái) / (2 * tốc độ cơ bản)
    # Điều này giúp turn_command nằm trong khoảng [-1, 1]
    # Đảm bảo BASE_SPEED_OFFSET không bằng 0 để tránh chia cho 0
    if BASE_SPEED_OFFSET == 0:
        turn_ratio = 0.0
    else:
        turn_ratio = (speeds_influence[1] - speeds_influence[0]) / (2 * BASE_SPEED_OFFSET)
    
    return np.clip(turn_ratio, -1.0, 1.0) # Đảm bảo nằm trong khoảng [-1, 1]

def blink_leds(leds_dict, robot_obj):
    """Makes one LED blink sequentially to show the robot is alive."""
    # Lấy thời gian hiện tại và tính toán chỉ số LED để bật
    current_led_idx = int(robot_obj.getTime() * 10) % len(LEDS_NAMES)
    for i, name in enumerate(LEDS_NAMES):
        leds_dict[name].set(1 if i == current_led_idx else 0)

def minimal_stuck_recovery(robot_obj, ts, sensors_dict, left_m, right_m):
    """Thực hiện quy trình phục hồi khi robot bị kẹt."""
    print("⚠️ Bị kẹt! Đang thực hiện phục hồi...")
    
    # Bước 1: Lùi lại một khoảng
    set_wheel_speeds(left_m, right_m, REVERSE_SPEED_RECOVERY, REVERSE_SPEED_RECOVERY)
    backup_steps = int(BACKUP_DURATION_MS_RECOVERY / ts)
    for _ in range(backup_steps):
        if robot_obj.step(ts) == -1: return False

    # Bước 2: Dừng một chút để ổn định và đọc lại cảm biến
    set_wheel_speeds(left_m, right_m, 0, 0)
    for _ in range(int(100 / ts)): # Dừng 100ms
        if robot_obj.step(ts) == -1: return False

    # Bước 3: Đọc lại cảm biến để quyết định hướng rẽ hợp lý sau khi lùi
    ps_values_after_backup = {name: sensors_dict[name].getValue() for name in DISTANCE_SENSORS_NAMES}
    # Chuẩn hóa lại để đưa vào logic so sánh hướng rẽ
    left_after_backup = np.clip(min(ps_values_after_backup['ps1'], ps_values_after_backup['ps2']), 0, SENSOR_MAX_RAW_VALUE) / SENSOR_MAX_RAW_VALUE
    right_after_backup = np.clip(min(ps_values_after_backup['ps5'], ps_values_after_backup['ps6']), 0, SENSOR_MAX_RAW_VALUE) / SENSOR_MAX_RAW_VALUE

    # Quyết định rẽ: ưu tiên rẽ sang phía thoáng hơn
    if left_after_backup > right_after_backup: # Trái thoáng hơn, rẽ trái
        set_wheel_speeds(left_m, right_m, -TURN_SPEED_RECOVERY, TURN_SPEED_RECOVERY)
    else: # Phải thoáng hơn hoặc cân bằng, rẽ phải
        set_wheel_speeds(left_m, right_m, TURN_SPEED_RECOVERY, -TURN_SPEED_RECOVERY)

    turn_steps = int(TURN_DURATION_MS_RECOVERY / ts)
    for _ in range(turn_steps):
        if robot_obj.step(ts) == -1: return False
        
    print("✅ Phục hồi xong, tiếp tục.")
    # Không dừng hẳn sau phục hồi. Để robot chạy lại logic chính ngay lập tức.
    return True

# --- Hàm Khởi tạo thiết bị ---
def init_devices():
    robot = Robot()
    timestep = int(robot.getBasicTimeStep() * TIME_STEP_FACTOR)

    dist_sensors = {} # Dùng dictionary để lưu theo tên
    for name in DISTANCE_SENSORS_NAMES:
        ds = robot.getDevice(name)
        ds.enable(timestep)
        dist_sensors[name] = ds

    ground_sensors = {} # Dùng dictionary để lưu theo tên
    for name in GROUND_SENSORS_NAMES:
        try:
            gs = robot.getDevice(name)
            gs.enable(timestep)
            ground_sensors[name] = gs
        except Exception as e:
            # print(f"Warning: Ground sensor '{name}' not found: {e}. Skipping.")
            pass 

    leds = {} # Dùng dictionary để lưu theo tên
    for name in LEDS_NAMES:
        leds[name] = robot.getDevice(name)

    left_motor = robot.getDevice('left wheel motor')
    right_motor = robot.getDevice('right wheel motor')
    left_motor.setPosition(float('inf'))
    right_motor.setPosition(float('inf'))
    left_motor.setVelocity(0.0)
    right_motor.setVelocity(0.0)

    return robot, timestep, dist_sensors, ground_sensors, leds, left_motor, right_motor

# --- Khởi tạo ---
robot, timestep, dist_sensors_dict, ground_sensors_dict, leds_dict, left_motor, right_motor = init_devices()

# --- Vòng lặp chính ---
print("Robot controller started with Braitenberg, Fuzzy Speed Control, and Stuck Recovery!")
while robot.step(timestep) != -1:
    # 1. Nhấp nháy đèn LED để robot trông "sống"
    blink_leds(leds_dict, robot)

    # 2. Đọc cảm biến
    # dist_vals_raw là các giá trị thô từ cảm biến khoảng cách (0-4095)
    # front_n, left_n, right_n là các giá trị đã chuẩn hóa (0-1) cho fuzzy logic
    dist_vals_raw = [s.getValue() for s in dist_sensors_dict.values()] # Lấy list các giá trị thô
    front_n, left_n, right_n = get_normalized_distance_readings(dist_sensors_dict)
    ground_vals_dict = get_ground_sensor_values(ground_sensors_dict)

    # 3. Xử lý vách đá (ưu tiên cao nhất)
    if check_cliff_detection(ground_vals_dict):
        # Lùi và rẽ trái khi phát hiện vách đá
        print("CLIFF DETECTED! Backing up and turning left.")
        set_wheel_speeds(left_motor, right_motor, -MAX_MOTOR_SPEED, -MAX_MOTOR_SPEED)
        # Sử dụng robot.step() để đợi trong mô phỏng
        for _ in range(int(0.2 * 1000 / timestep)): # Lùi 0.2 giây
            if robot.step(timestep) == -1: sys.exit(0) # Thoát nếu mô phỏng kết thúc
        
        set_wheel_speeds(left_motor, right_motor, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED)
        for _ in range(int(0.2 * 1000 / timestep)): # Rẽ 0.2 giây
            if robot.step(timestep) == -1: sys.exit(0) # Thoát nếu mô phỏng kết thúc
        continue # Bỏ qua phần điều khiển khác trong vòng lặp này và bắt đầu chu kỳ mới

    # 4. Logic phát hiện kẹt (Stuck Recovery)
    # Làm tròn giá trị để bỏ qua các dao động nhỏ của cảm biến
    current_sensor_reading = (round(front_n, 2), round(left_n, 2), round(right_n, 2))
    
    if current_sensor_reading == last_sensor_reading:
        stuck_counter += 1
    else:
        stuck_counter = 0
    last_sensor_reading = current_sensor_reading

    if stuck_counter > STUCK_THRESHOLD_COUNT:
        print(f"!!! ROBOT DETECTED AS STUCK !!! Triggering recovery. Counter: {stuck_counter}")
        if not minimal_stuck_recovery(robot, timestep, dist_sensors_dict, left_motor, right_motor):
            sys.exit(0) # Thoát vòng lặp nếu mô phỏng kết thúc trong quá trình phục hồi
        stuck_counter = 0 # Reset bộ đếm kẹt
        last_sensor_reading = None # Reset để robot không bị phát hiện kẹt lại ngay lập tức
        continue # Sau khi phục hồi, bắt đầu lại vòng lặp để robot đánh giá tình hình mới

    # 5. Tính toán tốc độ bằng Fuzzy Logic
    try:
        speed_simulation.input['front_distance'] = front_n
        speed_simulation.input['left_distance']  = left_n
        speed_simulation.input['right_distance'] = right_n
        
        speed_simulation.compute()
        speed_factor_output = speed_simulation.output['speed_factor']
        
        # Nếu speed_factor rất nhỏ, robot sẽ dừng hẳn
        MIN_FUZZY_SPEED_THRESHOLD = 0.05  # Ngưỡng của vùng 'stop'
        if speed_factor_output < MIN_FUZZY_SPEED_THRESHOLD: 
            current_base_speed = 0.0
            print("ROBOT STOPPED BY FUZZY LOGIC (too close to obstacle).")
        else:
            # Scale tốc độ cơ bản theo output của fuzzy. Nhân với 2 để có tốc độ nhanh hơn
            current_base_speed = speed_factor_output * BASE_SPEED_OFFSET * 2
            
            # Đảm bảo tốc độ không quá thấp nếu robot vẫn muốn di chuyển
            MIN_ALLOWED_SPEED_FOR_MOTION = 0.4  # Đã giảm ngưỡng để robot chạy ổn định, vừa phải
            if current_base_speed > 0 and current_base_speed < MIN_ALLOWED_SPEED_FOR_MOTION:
                current_base_speed = MIN_ALLOWED_SPEED_FOR_MOTION

    except Exception as e:
        print(f"Error computing fuzzy speed: {e}. Setting speed to 0.")
        current_base_speed = 0.0

    # 6. Tính toán hướng rẽ bằng Braitenberg
    turn_command = calculate_braitenberg_turn(dist_vals_raw) # Lấy lệnh rẽ từ Braitenberg

    # 7. Kết hợp tốc độ và hướng
    left_speed  = current_base_speed * (1 - turn_command)
    right_speed = current_base_speed * (1 + turn_command)
    
    # 8. Đặt tốc độ cho motor
    set_wheel_speeds(left_motor, right_motor, left_speed, right_speed)

    # In thông tin debug (tùy chọn)
    print(f"F_n={front_n:.2f}, L_n={left_n:.2f}, R_n={right_n:.2f} | Speed_Factor={speed_factor_output:.2f} | Turn_Cmd={turn_command:.2f} | VL={left_speed:.2f}, VR={right_speed:.2f}")

robot.cleanup()