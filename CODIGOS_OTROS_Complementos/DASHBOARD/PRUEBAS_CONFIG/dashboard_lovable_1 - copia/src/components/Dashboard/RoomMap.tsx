import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { MapPin, AlertTriangle } from "lucide-react";

interface Sensor {
  id: number;
  x: number;
  y: number;
  status: "normal" | "warning" | "alert";
  gasLevel: number;
}

interface RoomMapProps {
  sensors: Sensor[];
}

export const RoomMap = ({ sensors }: RoomMapProps) => {
  const getSensorColor = (status: string) => {
    switch (status) {
      case "normal":
        return "bg-success border-success/50";
      case "warning":
        return "bg-warning border-warning/50";
      case "alert":
        return "bg-destructive border-destructive/50 animate-pulse";
      default:
        return "bg-muted border-muted-foreground";
    }
  };

  const getZoneColor = (status: string) => {
    switch (status) {
      case "alert":
        return "bg-destructive/20";
      case "warning":
        return "bg-warning/10";
      default:
        return "transparent";
    }
  };

  return (
    <Card className="border-border bg-card/50 backdrop-blur-sm">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <MapPin className="h-5 w-5 text-primary" />
          Mapa del Salón - Sensores
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

          {/* Room elements */}
          <div className="absolute top-4 left-4 right-4 h-16 bg-secondary/50 rounded border border-border flex items-center justify-center">
            <span className="text-xs text-muted-foreground">Pizarra</span>
          </div>

          {/* Sensors */}
          {sensors.map((sensor) => (
            <div key={sensor.id}>
              {/* Alert zone */}
              {sensor.status === "alert" && (
                <div
                  className={`absolute rounded-full ${getZoneColor(sensor.status)} transition-all duration-500`}
                  style={{
                    left: `${sensor.x - 15}%`,
                    top: `${sensor.y - 15}%`,
                    width: '30%',
                    height: '30%',
                    zIndex: 0,
                  }}
                />
              )}
              
              {/* Sensor marker */}
              <div
                className="absolute transform -translate-x-1/2 -translate-y-1/2 cursor-pointer group"
                style={{
                  left: `${sensor.x}%`,
                  top: `${sensor.y}%`,
                  zIndex: 10,
                }}
              >
                <div className={`w-6 h-6 rounded-full ${getSensorColor(sensor.status)} border-2 flex items-center justify-center shadow-lg`}>
                  <div className="w-2 h-2 bg-foreground rounded-full" />
                </div>
                
                {/* Tooltip */}
                <div className="absolute bottom-full left-1/2 transform -translate-x-1/2 mb-2 opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none">
                  <div className="bg-popover border border-border rounded-lg px-3 py-2 shadow-lg min-w-max">
                    <div className="text-xs font-semibold text-foreground">Sensor {sensor.id}</div>
                    <div className="text-xs text-muted-foreground">{sensor.gasLevel} ppm</div>
                  </div>
                </div>
              </div>
            </div>
          ))}

          {/* Legend */}
          <div className="absolute bottom-4 right-4 bg-card/90 backdrop-blur-sm border border-border rounded-lg p-3 space-y-2">
            <div className="text-xs font-semibold text-foreground mb-2">Estado</div>
            <div className="flex items-center gap-2">
              <div className="w-3 h-3 rounded-full bg-success border border-success/50" />
              <span className="text-xs text-muted-foreground">Normal</span>
            </div>
            <div className="flex items-center gap-2">
              <div className="w-3 h-3 rounded-full bg-warning border border-warning/50" />
              <span className="text-xs text-muted-foreground">Advertencia</span>
            </div>
            <div className="flex items-center gap-2">
              <div className="w-3 h-3 rounded-full bg-destructive border border-destructive/50" />
              <span className="text-xs text-muted-foreground">Crítico</span>
            </div>
          </div>
        </div>
      </CardContent>
    </Card>
  );
};
