

clear;clc;close all
disp('INICIO')

%% LECTURA DE DATOS
[filename, pathname] = uigetfile('*.*');
filename = fullfile(pathname, filename); 
T = readtable(filename);
Tfiltered = T;


%Llenar valores de la tabla con valores por defecto decentes


%Convertir el threshold del axis transversal de viaje a 0
Tfiltered.lat = fillNaNsWithNextValue(Tfiltered.lat);
Tfiltered.long = fillNaNsWithNextValue(Tfiltered.long);
Tfiltered.kmh = fillWithZero(Tfiltered.kmh);
Tfiltered.accx = fillWithZero(Tfiltered.accx);
Tfiltered.accy = fillWithZero(Tfiltered.accy);

Tfiltered.accx = Tfiltered.accx * 0.1;
Tfiltered.accy = Tfiltered.accy * 0.1;

%Reducir el tamaño del sample para reducir el ruido
ts = downsample(Tfiltered.ts,10);
lat = downsample(Tfiltered.lat,10);
long = downsample(Tfiltered.long, 10);
kmh = downsample(Tfiltered.kmh, 10);
accx = downsample(abs(Tfiltered.accx), 10);
accy = downsample(abs(Tfiltered.accy), 10);

%Plots
figure;
plot(ts, kmh);
title("Tiempo vs Velocidad");
xlabel("milisegundos");
ylabel("km/h");

figure;
plot(ts, accx);
title("Tiempo vs Aceleracion (Axis X)");
xlabel("milisegundos");
ylabel("m/s²");

figure;
plot(ts, accy);
title("Tiempo vs Aceleracion (Axis Y)");
xlabel("milisegundos");
ylabel("m/s²");

figure;
mapcolors = getColors(kmh, 0, 50);
h = geoplot(lat,long, 'LineWidth', 5);
title("Velocidad en el recorido");
drawnow;
set(h.Edge,'ColorBinding','interpolated','ColorData',mapcolors);
a = colorbar;
caxis([0, 50]);
a.Label.String = "km/h";

figure;
mapcolors = getColors(accx, 0, 3);
h = geoplot(lat,long, 'LineWidth', 5);
title("Aceleraciones en el recorido (Axis X)");
a = colorbar;
a.Label.String = "m/s²";
caxis([0, 2]);
drawnow;
set(h.Edge,'ColorBinding','Interpolated','ColorData',mapcolors);

figure;
mapcolors = getColors(accy, 0, 3.5);
h = geoplot(lat,long, 'LineWidth', 5);
title("Aceleraciones en el recorido (Axis Y)");
a = colorbar;
a.Label.String = "m/s²";
caxis([0, 4]);
drawnow;
set(h.Edge,'ColorBinding','Interpolated','ColorData',mapcolors);



%Funcion que llena los valores NaN con ceros
function ret = fillWithZero(n) 
    n(isnan(n)) = 0;
    ret = n;
end

%trae los colores de un colormap basado en interpolacion de los valores de N 
%con el colormap Jet, se determinan los minimos y los maximos con minv y maxv

function ret = getColors(n, minv, maxv)
    cd = colormap(jet(512));
    it = interp1(linspace(minv, maxv, length(cd)), cd, n);
    cd = uint8(it.'*255); 
    cd(4,:) = 255;
    ret = cd;
end

%Funcion que llena los NaN de un array con el valor del siguiente punto de dato encontrad
function filledArray = fillNaNsWithNextValue(array);
    if ~isvector(array)
        error('Input must be a vector');
    end

    nanIndices = find(isnan(array));

    for i = 1:length(nanIndices)
        idx = nanIndices(i);
        
        nextIdx = find(~isnan(array(idx+1:end)), 1, 'first') + idx;
        
        if isempty(nextIdx) || nextIdx > length(array)
            warning('No next non-NaN value found for NaN at index %d', idx);
            array(idx) = NaN;
        else
            array(idx) = array(nextIdx);
        end
    end

    filledArray = array;
end
