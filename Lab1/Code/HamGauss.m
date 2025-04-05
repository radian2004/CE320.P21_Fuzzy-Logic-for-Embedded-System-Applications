sigma = 5;
cold_gauss = gaussmf(x, [sigma, 10]);
normal_gauss = gaussmf(x, [sigma, 25]);
hot_gauss = gaussmf(x, [sigma, 40]);
plot(x, cold_gauss, 'b', 'LineWidth', 2);
hold on;
plot(x, normal_gauss, 'g', 'LineWidth', 2);
plot(x, hot_gauss, 'r', 'LineWidth', 2);
hold off;
xlabel('Nhiệt độ (°C)');
ylabel('Mức độ thành viên');
title('Hàm thành viên Gaussian');legend('Lạnh', 'Bình thường', 'Nóng');
grid on;