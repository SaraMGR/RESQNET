import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { MapPin } from "lucide-react";

interface Sensor {
  id: number;
  x: number;
  y: number;
  z: number;
  status: "normal" | "warning" | "alert" | "critical";
  gasLevel: number;
  tvocLevel: number; // Agregado para que coincida con tu Index.tsx
  // 🟢 AÑADIR LOS NUEVOS CAMPOS DEL PAYLOAD DE AIRE
  temperatura: number;
  humedad: number;
  aqi: number;
  prom_eco2: number;
}

interface RoomMapProps {
  sensors: Sensor[];
}

// Constantes de posición fijas para los nodos
const SENSOR_POSITIONS = {
    1: { x: 15, y: 90, name: "Nodo 1 (Esquina Izq.)" },
    2: { x: 85, y: 90, name: "Nodo 2 (Esquina Der.)" },
    3: { x: 50, y: 35, name: "Nodo 3 (Centro)" },
    // Los nodos 4 y 5 se ignoran en el mapa por la lógica de filtrado de mainSensors
};

export const RoomMap = ({ sensors }: RoomMapProps) => {
  const getSensorColor = (status: string) => {
    switch (status) {
      case "normal":
        return "bg-success border-success/50";
      case "warning":
        return "bg-warning border-warning/50";
      case "alert":
        return "bg-alert border-alert/50 animate-pulse"; // Usamos 'alert' aquí
      case "critical": 
        return "bg-destructive border-destructive/50 animate-ping";
      default:
        return "bg-muted border-muted-foreground";
    }
  };

  const getZoneColor = (status: string) => {
    switch (status) {
      case "critical":
        return "bg-destructive/20"; // Fondo de pulso rojo/destructivo para alertas
      case "alert":
        return "bg-alert/20"; // Fondo de pulso rojo/destructivo para alertas
      case "warning":
        return "bg-warning/20";
      case "normal":
        return "bg-success/20"; // 🟢 Fondo de pulso azul/primary para actividad normal
      default:
        return "transparent";
    }
  };
    
  // Filtramos los sensores para asegurarnos de solo usar los nodos definidos en SENSOR_POSITIONS
  const mainSensors = sensors.filter(s => s.id in SENSOR_POSITIONS);

  return (
    <Card className="border-border bg-card/50 backdrop-blur-sm">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <MapPin className="h-5 w-5 text-primary" />
          Mapa del Salón - Ubicación de nodos
        </CardTitle>
      </CardHeader>
      <CardContent>
        <div className="relative w-full h-80 bg-secondary/30 rounded-lg border border-border overflow-hidden">
          {/* Grid background */}
          <div className="absolute inset-0" style={{
            backgroundImage: `
              linear-gradient(to right, hsl(var(--border)) 1px, transparent 1px),
              linear-gradient(to bottom, hsl(var(--border)) 1px, transparent 1px)
            `,
            backgroundSize: '40px 40px'
          }} />

          {/* Room elements (Pizarra) */}
          <div className="absolute top-4 left-4 right-4 h-16 bg-secondary/50 rounded border border-border flex items-center justify-center">
            <span className="text-xs text-muted-foreground">Pizarra</span>
          </div>

          {/* Sensores */}
          {mainSensors.map((sensor) => {
                const pos = SENSOR_POSITIONS[sensor.id as keyof typeof SENSOR_POSITIONS];
                if (!pos) return null;

                const sensorName = pos.name || `Sensor ${sensor.id}`;
                
                // 🟢 LÓGICA DE PULSO CENTRALIZADA
                const pulseColor = getZoneColor(sensor.status);
                const pulseAnimationClass = (sensor.status === "critical") 
                                            ? "animate-ping" 
                                            : (sensor.status === "alert" || sensor.status === "normal"
                                               ? "animate-pulse" // Pulso suave para normal y alert
                                               : "");
                
                return (
            <div key={sensor.id}>
              
              {/* 🚨 Zona de Pulso (Visible y animada para Normal, Alert y Critical) */}
              {pulseColor !== "transparent" && (
                <div
                  className={`absolute rounded-full ${pulseColor} ${pulseAnimationClass} transition-all duration-500`}
                  style={{
                    left: `${pos.x - 7}%`,
                    top: `${pos.y - 7}%`,
                    width: '14%',
                    height: '14%',
                    zIndex: 0,
                  }}
                />
              )}
              
              {/* Sensor marker */}
              <div
                className="absolute transform -translate-x-1/2 -translate-y-1/2 cursor-pointer group"
                style={{
                  left: `${pos.x}%`, 
                  top: `${pos.y}%`, 
                  zIndex: 10,
                }}
              >
                <div className={`w-6 h-6 rounded-full ${getSensorColor(sensor.status)} border-2 flex items-center justify-center shadow-lg`}>
                  <div className="w-2 h-2 bg-foreground rounded-full" />
                </div>
                
                {/* Tooltip */}
                <div className="absolute bottom-full left-1/2 transform -translate-x-1/2 mb-2 opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none">
                  <div className="bg-popover border border-border rounded-lg px-3 py-2 shadow-lg min-w-max">
                    <div className="text-xs font-semibold text-foreground">{sensorName}</div>

                    {/* Ejes X, Y, Z */}
                    <div className="flex gap-2 text-xs text-muted-foreground">
                        {/* X */}
                        <div>X:{sensor.x.toFixed(2)}</div>
                        {/* Y */}
                        <div>Y:{sensor.y.toFixed(2)}</div>
                        {/* Z */}
                        <div>Z: {sensor.z.toFixed(2)}</div>
                    </div>

                    {/* Calidad del Aire */}
                    <div className="flex gap-2 text-xs text-muted-foreground">
                        {/* Indicador de AQI */}
                        <div>AQI:{sensor.aqi}</div>
                        {/* Indicador de TVOC */}
                        <div>TVOC:{sensor.tvocLevel} ppb</div>
                        {/* Indicador de eCO2 */}
                        <div>eCO2: {sensor.gasLevel} ppm</div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          )})}

          {/* Legend */}
          <div className="absolute bottom-20 right-4 bg-card/90 backdrop-blur-sm border border-border rounded-lg p-3 space-y-2">
            <div className="text-xs font-semibold text-foreground mb-2">Estado</div>
            <div className="flex items-center gap-2">
              <div className="w-3 h-3 rounded-full bg-success border border-success/50" />
              <span className="text-xs text-muted-foreground">Normal (Activo)</span>
            </div>
            <div className="flex items-center gap-2">
              <div className="w-3 h-3 rounded-full bg-warning border border-warning/50" />
              <span className="text-xs text-muted-foreground">Advertencia</span>
            </div>
            <div className="flex items-center gap-2">
              <div className="w-3 h-3 rounded-full bg-destructive border border-destructive/50" />
              <span className="text-xs text-muted-foreground">Alerta/Crítico</span>
            </div>
          </div>
        </div>
      </CardContent>
    </Card>
  );
};
