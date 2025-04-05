xT = 0:0.1:50; % Nhiệt độ từ 0 đến 50°C
xP = 0:1:100; % Công suất từ 0 đến 100%% Hàm thành viên cho nhiệt độ
cold = trimf(xT, [0, 0, 20]);
normal = trimf(xT, [10, 25, 40]);
hot = trimf(xT, [30, 50, 50]);
% Hàm thành viên cho công suất quạt
low = trimf(xP, [0, 0, 50]);
medium = trimf(xP, [25, 50, 75]);
high = trimf(xP, [50, 100, 100]);
figure;
subplot(1,2,1);
plot(xT, cold, 'b', xT, normal, 'g', xT, hot, 'r', 'LineWidth', 2);
xlabel('Nhiệt độ (°C)');
ylabel('Mức độ thành viên');
title('Hàm thành viên nhiệt độ');
legend('Lạnh', 'Bình thường', 'Nóng');
grid on;
subplot(1,2,2);
plot(xP, low, 'b', xP, medium, 'g', xP, high, 'r', 'LineWidth', 2);
xlabel('Công suất (%)');
ylabel('Mức độ thành viên');
title('Hàm thành viên công suất');
legend('Thấp', 'Trung bình', 'Cao');
grid on;