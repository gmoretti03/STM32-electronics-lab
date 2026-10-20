porta = "COM10"; % La porta seriale utilizzata per comunicare con il microcontrollore
serial = serialport(porta, 115200, "Timeout", 1); % Creo l'oggetto seriale per la comunicazione

N = 100; % Numero di campioni da leggere
P = 20;  % Parametro di riordino
K = N-P;
Na = 10;

write(serial, 'c', 'char'); % Invia il carattere 'c' per avviare la comunicazione

% Variabili di controllo del ciclo
i = 1;
disp('sono qui')

%PROBLEMA NEL RIORDINO DEI DATI
while true
    if serial.NumBytesAvailable > 0 % Controlla se ci sono dati disponibili
        dati = read(serial, 1000, "uint16"); % Leggi 100 dati (uint16)
        T = read(serial, 10, "uint16")-2; % Leggi u dato che indica la posizione
        disp('ciao')
        if i < Na+1
        % Gestione del riordino in base a T
       %disp (dati)
            if (T(i) - P) > 0
                B = T(i)-P-1;
                primo = dati(B+2:end); % Prima parte del riordino
                secondo = dati(1:B+1);    % Seconda parte del riordino
            elseif (T(i) - P) < 0
                B = T(i)+K-1;
                primo = dati(B+2:end); % Prima parte del riordino
                secondo = dati(1:B+1);   % Seconda parte del riordino
            else
                primo = dati(K:N); % Prima parte del riordino
                secondo = dati(1:K);    % Seconda parte del riordino
            end

        % Unisco le due parti riordinate
            dati_ordinati = [primo, secondo];

        % Conversione dei dati in valori di tensione (0-3.3V)
            conversione = dati_ordinati * 3.3 / 65535;


        % Aggiungi un piccolo delay per evitare sovraccarico della CPU
            pause(0.01);

        % Visualizza i dati riordinati
        
            plot(conversione);
            
            xlabel('Indice');
            ylabel('Tensione (V)');
        
        % Contatore per visualizzare ogni 100 iterazioni
        i = i + 1;
        end
    end
    pause(.01)
end
