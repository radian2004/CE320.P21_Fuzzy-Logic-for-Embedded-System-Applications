x = 0:0.1:50; % Miền giá trị nhiệt độ từ 0 đến 50°C
cold_trap = trapmf(x, [0, 0, 10, 20]);
normal_trap = trapmf(x, [10, 20, 30, 40]);
hot_trap = trapmf(x, [30, 40, 50, 50]);
plot(x, cold_trap, 'b', 'LineWidth', 2);
hold on;
plot(x, normal_trap, 'g', 'LineWidth', 2);
plot(x, hot_trap, 'r', 'LineWidth', 2);
hold off;
xlabel('Nhiệt độ (°C)');
ylabel('Mức độ thành viên');
title('Hàm thành viên hình thang');
legend('Lạnh', 'Bình thường', 'Nóng');
grid on;