%% Bước 1: Khởi tạo hệ thống FIS theo đề bài tự động hóa tưới cây
fis = mamfis('Name', 'IrrigationSystem');

%% Bước 2: Định nghĩa biến đầu vào "KhoDat" (Mức độ khô của đất) với miền [0 60]
fis = addInput(fis, [0 60], 'Name', 'KhoDat');
% Hàm thành viên tam giác: Thấp: [0, 10, 20], TrungBinh: [20, 30, 40], Cao: [40, 50, 60]
fis = addMF(fis, 'KhoDat', 'trimf', [0 10 20], 'Name', 'Thap');
fis = addMF(fis, 'KhoDat', 'trimf', [20 30 40], 'Name', 'TrungBinh');
fis = addMF(fis, 'KhoDat', 'trimf', [40 50 60], 'Name', 'Cao');

%% Bước 3: Định nghĩa biến đầu vào "LuongMua" (Dự báo lượng mưa) với miền [0 30]
fis = addInput(fis, [0 30], 'Name', 'LuongMua');
% Hàm thành viên: Thap: [0, 5, 10], TrungBinh: [10, 12.5, 15], Cao: [15, 22.5, 30]
fis = addMF(fis, 'LuongMua', 'trimf', [0 5 10], 'Name', 'Thap');
fis = addMF(fis, 'LuongMua', 'trimf', [10 12.5 15], 'Name', 'TrungBinh');
fis = addMF(fis, 'LuongMua', 'trimf', [15 22.5 30], 'Name', 'Cao');

%% Bước 4: Định nghĩa biến đầu vào "DoAm" (Độ ẩm) với miền [0 100]
fis = addInput(fis, [0 100], 'Name', 'DoAm');
% Hàm thành viên: Thap: [0, 35, 70], TrungBinh: [70, 77.5, 85], Cao: [85, 92.5, 100]
fis = addMF(fis, 'DoAm', 'trimf', [0 35 70], 'Name', 'Thap');
fis = addMF(fis, 'DoAm', 'trimf', [70 77.5 85], 'Name', 'TrungBinh');
fis = addMF(fis, 'DoAm', 'trimf', [85 92.5 100], 'Name', 'Cao');

%% Bước 5: Định nghĩa biến đầu vào "NhietDo" (Nhiệt độ) với miền [0 40]
fis = addInput(fis, [0 40], 'Name', 'NhietDo');
% Hàm thành viên: Thap: [0, 5, 10], TrungBinh: [10, 17.5, 25], Cao: [25, 32.5, 40]
fis = addMF(fis, 'NhietDo', 'trimf', [0 5 10], 'Name', 'Thap');
fis = addMF(fis, 'NhietDo', 'trimf', [10 17.5 25], 'Name', 'TrungBinh');
fis = addMF(fis, 'NhietDo', 'trimf', [25 32.5 40], 'Name', 'Cao');

%% Bước 6: Định nghĩa biến đầu ra "Tuoi" (Mức độ tưới) với miền [0 100]
fis = addOutput(fis, [0 100], 'Name', 'Tuoi');
% Hàm thành viên cho đầu ra: KhongTuoi: [0, 0, 50], TuoiNhe: [25, 50, 75], TuoiManh: [50, 100, 100]
fis = addMF(fis, 'Tuoi', 'trimf', [0 0 50], 'Name', 'KhongTuoi');
fis = addMF(fis, 'Tuoi', 'trimf', [25 50 75], 'Name', 'TuoiNhe');
fis = addMF(fis, 'Tuoi', 'trimf', [50 100 100], 'Name', 'TuoiManh');

%% Bước 7: Xây dựng tập luật mờ dựa trên kinh nghiệm
% Lưu ý: Sử dụng toán tử Min để kết hợp các điều kiện (mặc định của Mamdani) và Max để kết hợp các luật.
% Một số luật mẫu:
% Luật 1: Nếu KhoDat là Thap và LuongMua là Cao và DoAm là Cao thì Tuoi là KhongTuoi.
rule1 = "KhoDat==Thap & LuongMua==Cao & DoAm==Cao => Tuoi=KhongTuoi";
% Luật 2: Nếu KhoDat là Cao và LuongMua là Thap thì Tuoi là TuoiManh.
rule2 = "KhoDat==Cao & LuongMua==Thap => Tuoi=TuoiManh";
% Luật 3: Nếu KhoDat là TrungBinh và LuongMua==TrungBinh thì Tuoi là TuoiNhe.
rule3 = "KhoDat==TrungBinh & LuongMua==TrungBinh => Tuoi=TuoiNhe";
% Luật 4: Nếu NhietDo là Cao thì Tuoi là TuoiManh.
rule4 = "NhietDo==Cao => Tuoi=TuoiManh";
% Luật 5: Nếu NhietDo là Thap và DoAm là Cao thì Tuoi là KhongTuoi.
rule5 = "NhietDo==Thap & DoAm==Cao => Tuoi=KhongTuoi";

fis = addRule(fis, [rule1, rule2, rule3, rule4, rule5]);

%% Bước 8: Thực hiện suy luận mờ với các giá trị đầu vào cho tình huống:
% Khi KhoDat = 25, LuongMua = 12, DoAm = 90, NhietDo = 25
input_values = [25, 12, 90, 25];
output_value = evalfis(fis, input_values);
fprintf("Gia tri Tuoi (mức độ tưới) là: %f\n", output_value);

%% Bước 9: Trực quan hóa các hàm thành viên
figure;
subplot(5,1,1);
plotmf(fis, 'input', 1);
title('Membership Functions for Soil Dryness (KhoDat)');

subplot(5,1,2);
plotmf(fis, 'input', 2);
title('Membership Functions for Rain Forecast (LuongMua)');

subplot(5,1,3);
plotmf(fis, 'input', 3);
title('Membership Functions for Humidity (DoAm)');

subplot(5,1,4);
plotmf(fis, 'input', 4);
title('Membership Functions for Temperature (NhietDo)');

subplot(5,1,5);
plotmf(fis, 'output', 1);
title('Membership Functions for Irrigation Level (Tuoi)');

%% Mở Fuzzy Logic Designer để xem hệ thống
writeFIS(fis, 'IrrigationSystem.fis');
fuzzyLogicDesigner('IrrigationSystem.fis');

%% Thông tin tác giả
annotation('textbox', [0.75, 0.01, 0.2, 0.05], 'String', 'Nguyễn Thanh An', ...
           'EdgeColor', 'none', 'HorizontalAlignment', 'right', 'FontSize', 10);