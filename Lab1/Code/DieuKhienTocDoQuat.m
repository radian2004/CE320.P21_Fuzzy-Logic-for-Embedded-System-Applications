xT = 0:0.1:50; % Nhiệt độ từ 0 đến 50°C
xP = 0:1:100; % Tốc độ quạt từ 0 đến 100%

% Tập mờ cho biến nhiệt độ
cold = trimf(xT, [0, 0, 15]);       % Lạnh: [0,0,15]
normal = trimf(xT, [10, 25, 40]);    % Bình thường: [10,25,40]
hot = trimf(xT, [30, 50, 50]);       % Nóng: [30,50,50]

% Tập mờ cho biến tốc độ quạt
slow = trimf(xP, [0, 0, 50]);        % Chậm: [0,0,50]
medium = trimf(xP, [25, 50, 75]);    % Trung bình: [25,50,75]
fast = trimf(xP, [50, 100, 100]);    % Nhanh: [50,100,100]

figure;

% Subplot cho nhiệt độ
subplot(1,2,1);    % Chia đồ thị thành 1 hàng, 2 cột, vẽ ở vị trí 1
plot(xT, cold, 'b', xT, normal, 'g', xT, hot, 'r', 'LineWidth', 2);
xlabel('Nhiệt độ (°C)'); %Nhãn trục x
ylabel('Mức độ thành viên'); %Nhãn trục y
title('Hàm thành viên nhiệt độ'); %Tiêu đề cho đồ thị
legend('Lạnh', 'Bình thường', 'Nóng');  %Chú thích
grid on; %Hiển thị lưới

% Subplot cho tốc độ quạt
subplot(1,2,2);   % Chia đồ thị thành 1 hàng, 2 cột, vẽ ở vị trí 2
plot(xP, slow, 'b', xP, medium, 'g', xP, fast, 'r', 'LineWidth', 2);
xlabel('Tốc độ quạt (%)'); %Nhãn trục x
ylabel('Mức độ thành viên'); %Nhãn trục y
title('Hàm thành viên tốc độ quạt'); %Tiêu đề cho đồ thị
legend('Chậm', 'Trung bình', 'Nhanh'); %Chú thích
grid on; %Hiển thị lưới

% Hiển thị tên tác giả trên hình
annotation('textbox', [0.75, 0.01, 0.2, 0.05], 'String', 'Nguyễn Thanh An', ...
           'EdgeColor', 'none', 'HorizontalAlignment', 'right', 'FontSize', 10);