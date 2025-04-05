fis = mamfis("Name", "ControlSystem");

fis = addInput(fis, [0 100], "Name", "Temperature");
fis = addMF(fis, "Temperature", "trimf", [0 0 50], "Name", "Cold");
fis = addMF(fis, "Temperature", "trimf", [25 50 75], "Name", "Warm");
fis = addMF(fis, "Temperature", "trimf", [50 100 100], "Name", "Hot");

fis = addInput(fis, [0 100], "Name", "Humidity");
fis = addMF(fis, "Humidity", "trimf", [0 0 40], "Name", "Low");
fis = addMF(fis, "Humidity", "trimf", [30 50 70], "Name", "Medium");
fis = addMF(fis, "Humidity", "trimf", [60 100 100], "Name", "High");

fis = addOutput(fis, [0 10], "Name", "FanSpeed");
fis = addMF(fis, "FanSpeed", "trimf", [0 0 5], "Name", "Low");
fis = addMF(fis, "FanSpeed", "trimf", [2.5 5 7.5], "Name", "Medium");
fis = addMF(fis, "FanSpeed", "trimf", [5 10 10], "Name", "High");

ruleList = [...
    "If Temperature is Cold and Humidity is Low then FanSpeed is Low"; 
    "If Temperature is Cold and Humidity is Medium then FanSpeed is Medium";
    "If Temperature is Cold and Humidity is High then FanSpeed is Medium"
    "If Temperature is Warm and Humidity is Low then FanSpeed is Medium";
    "If Temperature is Warm and Humidity is Medium then FanSpeed is Medium";
    "If Temperature is Warm and Humidity is High then FanSpeed is Medium";
    "If Temperature is Hot and Humidity is Low then FanSpeed is High";
    "If Temperature is Hot and Humidity is Medium then FanSpeed is High";
    "If Temperature is Hot and Humidity is High then FanSpeed is High"];

fis = addRule(fis, ruleList);

humidity = 50;
temp = 30; % Giá trị nhiệt độ đầu vào
output_value = evalfis(fis, [temp, humidity]);

% Hiển thị kết quả
fprintf("Fan Speed Output: %f\n", output_value);

plotmf(fis, "input", 1); 
plotmf(fis, "output", 1);