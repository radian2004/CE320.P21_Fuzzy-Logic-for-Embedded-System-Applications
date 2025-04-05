x = 0:0.1:100; % Miền giá trị nhiệt độ (0 đến 100°C)
% Hàm thành viên Gaussian
sigma = 10; % Độ rộng của hàm Gaussian
cold_gauss = gaussmf(x, [sigma, 20]);     
normal_gauss = gaussmf(x, [sigma, 50]);
hot_gauss = gaussmf(x, [sigma, 80]);

% Hàm thành viên tam giác
cold_trimf = trimf(x, [0, 0, 40]);
normal_trimf = trimf(x, [20, 50, 80]);
hot_trimf = trimf(x, [60, 100, 100]);

% Hàm thành viên hình thang
cold_trapmf = trapmf(x, [0, 0, 20, 40]);
normal_trapmf = trapmf(x, [20, 40, 60, 80]);
hot_trapmf = trapmf(x, [60, 80, 100, 100]);

% Vẽ đồ thị
figure;
plot(x, cold_gauss, 'b', 'LineWidth', 2); 
hold on;   % Giữ nguyên đồ thị hiện tại
plot(x, normal_gauss, 'g', 'LineWidth', 2);
plot(x, hot_gauss, 'r', 'LineWidth', 2);

plot(x, cold_trimf, 'b--', 'LineWidth', 2);
plot(x, normal_trimf, 'g--', 'LineWidth', 2);
plot(x, hot_trimf, 'r--', 'LineWidth', 2);

plot(x, cold_trapmf, 'b:', 'LineWidth', 2);
plot(x, normal_trapmf, 'g:', 'LineWidth', 2);
plot(x, hot_trapmf, 'r:', 'LineWidth', 2);

hold off;
xlabel('Nhiệt độ (°C)'); %Nhãn trục x
ylabel('Mức độ thành viên'); %Nhãn trục y
title('So sánh các hàm thành viên by Nguyễn Thanh An'); %Tiêu đề cho đồ thị
legend('Gauss - Lạnh', 'Gauss - Bình thường', 'Gauss - Nóng', ...
       'Tam giác - Lạnh', 'Tam giác - Bình thường', 'Tam giác - Nóng', ...
       'Hình thang - Lạnh', 'Hình thang - Bình thường', 'Hình thang - Nóng'); %Chú thích
grid on; %Hiển thị lưới
% Hiển thị tên tác giả trên hình
annotation('textbox', [0.75, 0.01, 0.2, 0.05], 'String', 'Nguyễn Thanh An', ...
           'EdgeColor', 'none', 'HorizontalAlignment', 'right', 'FontSize', 10);