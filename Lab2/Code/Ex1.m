% Bước 1: Khởi tạo hệ thống FIS
fis = mamfis('Name', 'ControlSystem');

% Bước 2: Định nghĩa biến đầu vào "Temperature"
fis = addInput(fis, [0 100], 'Name', 'Temperature');
fis = addMF(fis, 'Temperature', 'trimf', [0 0 50], 'Name', 'Cold');
fis = addMF(fis, 'Temperature', 'trimf', [25 50 75], 'Name', 'Warm');
fis = addMF(fis, 'Temperature', 'trimf', [50 100 100], 'Name', 'Hot');

% Bước 3: Định nghĩa biến đầu vào "Humidity" (Độ ẩm)
fis = addInput(fis, [0 100], 'Name', 'Humidity');
fis = addMF(fis, 'Humidity', 'trimf', [0 0 50], 'Name', 'Dry');  
fis = addMF(fis, 'Humidity', 'trimf', [25 50 75], 'Name', 'Normal');  
fis = addMF(fis, 'Humidity', 'trimf', [50 100 100], 'Name', 'Humid');  

% Bước 4: Định nghĩa biến đầu ra "FanSpeed"
fis = addOutput(fis, [0 10], 'Name', 'FanSpeed');
fis = addMF(fis, 'FanSpeed', 'trimf', [0 0 5], 'Name', 'Low');
fis = addMF(fis, 'FanSpeed', 'trimf', [2.5 5 7.5], 'Name', 'Medium');
fis = addMF(fis, 'FanSpeed', 'trimf', [5 10 10], 'Name', 'High');

% Bước 5: Xây dựng tập luật mờ (Temperature và Humidity ảnh hưởng FanSpeed)
rule1 = "Temperature==Cold & Humidity==Dry => FanSpeed=Low";
rule2 = "Temperature==Cold & Humidity==Normal => FanSpeed=Low";
rule3 = "Temperature==Cold & Humidity==Humid => FanSpeed=Low";

rule4 = "Temperature==Warm & Humidity==Dry => FanSpeed=Low";
rule5 = "Temperature==Warm & Humidity==Normal => FanSpeed=Medium";
rule6 = "Temperature==Warm & Humidity==Humid => FanSpeed=Medium";

rule7 = "Temperature==Hot & Humidity==Dry => FanSpeed=Medium";
rule8 = "Temperature==Hot & Humidity==Normal => FanSpeed=High";
rule9 = "Temperature==Hot & Humidity==Humid => FanSpeed=High";

fis = addRule(fis, [rule1, rule2, rule3, rule4, rule5, rule6, rule7, rule8, rule9]);

% Bước 6: Thực hiện suy luận mờ với đầu vào cụ thể
input_values = [80, 90]; % Giá trị nhiệt độ và độ ẩm đầu vào
output_value = evalfis(fis, input_values);
fprintf("Fan Speed Output: %f\n", output_value);

% Bước 7: Trực quan hóa tập mờ
figure;
subplot(3,1,1);
plotmf(fis, 'input', 1);
title('Membership Functions for Temperature');

subplot(3,1,2);
plotmf(fis, 'input', 2);
title('Membership Functions for Humidity');

subplot(3,1,3);
plotmf(fis, 'output', 1);
title('Membership Functions for FanSpeed');

% Mở Fuzzy Logic Designer để xem hệ thống
writeFIS(fis, 'ControlSystem.fis');
fuzzyLogicDesigner('ControlSystem.fis');

annotation('textbox', [0.75, 0.01, 0.2, 0.05], 'String', 'Nguyễn Thanh An', ...
           'EdgeColor', 'none', 'HorizontalAlignment', 'right', 'FontSize', 10);