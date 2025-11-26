import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Activity } from "lucide-react";
import { PieChart, Pie, Cell, ResponsiveContainer } from "recharts";

interface GasLevelMeterProps {
  level: number;
}

export const GasLevelMeter = ({ level }: GasLevelMeterProps) => {
  const maxLevel = 1000;
  const percentage = (level / maxLevel) * 100;
  
  const getStatus = () => {
    if (level < 300) return { color: "#10b981", text: "Normal", bg: "bg-success/20" };
    if (level < 600) return { color: "#f59e0b", text: "Advertencia", bg: "bg-warning/20" };
    return { color: "#ef4444", text: "Crítico", bg: "bg-destructive/20" };
  };

  const status = getStatus();
  const data = [
    { value: level },
    { value: maxLevel - level },
  ];

  return (
    <Card className="border-border bg-card/50 backdrop-blur-sm">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Activity className="h-5 w-5 text-primary" />
          Nivel de Gas
        </CardTitle>
      </CardHeader>
      <CardContent>
        <div className="flex items-center justify-between">
          <div className="relative w-40 h-40">
            <ResponsiveContainer width="100%" height="100%">
              <PieChart>
                <Pie
                  data={data}
                  cx="50%"
                  cy="50%"
                  innerRadius={50}
                  outerRadius={70}
                  startAngle={90}
                  endAngle={-270}
                  dataKey="value"
                >
                  <Cell fill={status.color} />
                  <Cell fill="#1e293b" />
                </Pie>
              </PieChart>
            </ResponsiveContainer>
            <div className="absolute inset-0 flex items-center justify-center flex-col">
              <div className="text-3xl font-bold text-foreground">{level}</div>
              <div className="text-xs text-muted-foreground">ppm</div>
            </div>
          </div>
          
          <div className="flex-1 ml-6 space-y-4">
            <div>
              <div className="flex justify-between items-center mb-2">
                <span className="text-sm text-muted-foreground">Estado</span>
                <span className={`px-3 py-1 rounded-full text-xs font-semibold ${status.bg} text-foreground`}>
                  {status.text}
                </span>
              </div>
              <div className="w-full bg-secondary rounded-full h-3 overflow-hidden">
                <div
                  className="h-full transition-all duration-500 rounded-full"
                  style={{
                    width: `${percentage}%`,
                    backgroundColor: status.color,
                  }}
                />
              </div>
            </div>
            <div className="grid grid-cols-3 gap-2 text-xs">
              <div className="text-center">
                <div className="text-success font-semibold">0-300</div>
                <div className="text-muted-foreground">Normal</div>
              </div>
              <div className="text-center">
                <div className="text-warning font-semibold">300-600</div>
                <div className="text-muted-foreground">Alerta</div>
              </div>
              <div className="text-center">
                <div className="text-destructive font-semibold">600+</div>
                <div className="text-muted-foreground">Crítico</div>
              </div>
            </div>
          </div>
        </div>
      </CardContent>
    </Card>
  );
};
