import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Waves } from "lucide-react";

interface VibrationIndicatorProps {
  intensity: "leve" | "moderada" | "fuerte";
  value: number;
}

export const VibrationIndicator = ({ intensity, value }: VibrationIndicatorProps) => {
  const getIntensityConfig = () => {
    switch (intensity) {
      case "leve":
        return { color: "text-success", bg: "bg-success/20", bars: 1 };
      case "moderada":
        return { color: "text-warning", bg: "bg-warning/20", bars: 2 };
      case "fuerte":
        return { color: "text-destructive", bg: "bg-destructive/20", bars: 3 };
    }
  };

  const config = getIntensityConfig();

  return (
    <Card className="border-border bg-card/50 backdrop-blur-sm">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Waves className="h-5 w-5 text-primary" />
          Detección de Vibración
        </CardTitle>
      </CardHeader>
      <CardContent>
        <div className="space-y-4">
          <div className="flex items-center justify-between">
            <div>
              <div className="text-3xl font-bold text-foreground capitalize">{intensity}</div>
              <div className="text-sm text-muted-foreground">{value.toFixed(2)} m/s²</div>
            </div>
            <div className={`px-4 py-2 rounded-lg ${config.bg}`}>
              <div className="flex items-end gap-1 h-12">
                {[1, 2, 3].map((bar) => (
                  <div
                    key={bar}
                    className={`w-3 rounded-t transition-all duration-300 ${
                      bar <= config.bars ? config.color.replace('text-', 'bg-') : 'bg-secondary'
                    }`}
                    style={{
                      height: `${bar * 33}%`,
                      animation: bar <= config.bars ? 'pulse 1s ease-in-out infinite' : 'none',
                    }}
                  />
                ))}
              </div>
            </div>
          </div>

          <div className="grid grid-cols-3 gap-2">
            {["leve", "moderada", "fuerte"].map((level) => (
              <button
                key={level}
                className={`px-3 py-2 rounded-lg text-xs font-medium transition-all ${
                  intensity === level
                    ? `${config.bg} ${config.color}`
                    : "bg-secondary text-muted-foreground"
                }`}
              >
                {level.charAt(0).toUpperCase() + level.slice(1)}
              </button>
            ))}
          </div>
        </div>
      </CardContent>
    </Card>
  );
};
