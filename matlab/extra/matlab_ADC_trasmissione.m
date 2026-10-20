porta = "COM10"; %la porta che vogliamo utilizzare per comunicare con la 
%seriale del microcontrollore usart3 è la com6. La definisco

serial = serialport(porta, 115200); %ho creato l'oggetto che mi permette di 
%comunicare tra matlab e il microcontrollore

%ora possiamo scrivere e ricevere dalla seriale:
%per leggere uso read(); 

c = "c";
write(serial, c, "char");

calibrazione = read(serial, 3, 'uint16') %vettore = tre valori da 16 

%valoriV = read(serial, 10, 'uint16') %gli sto mandando 2 

for i=1:10
valoriOscilloscopio = read(serial, 1000, 'uint16') % da interpretare come unit16


%temperatura = 30 + (valoriOscilloscopio-calibrazione(1))*80 ./ (calibrazione(2)-calibrazione(1))
%tensione_ref = calibrazione(3)*3.3 ./valoriOscilloscopio
tensione = valoriOscilloscopio*3.3 ./ 65000

%plot(tensione)
%hold on;
plot(tensione)
end;
delete(serial) %cancello la seriale altrimenti matlab cerca continuamente 
%di comunicare con il nulla