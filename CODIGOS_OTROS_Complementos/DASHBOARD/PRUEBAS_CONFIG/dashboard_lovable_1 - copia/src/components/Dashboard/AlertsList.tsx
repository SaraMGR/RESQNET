import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { AlertTriangle, Activity, Waves, CheckCircle, Siren } from "lucide-react";
import { ScrollArea } from "@/components/ui/scroll-area";

interface Alert {
  id: number;
  type: "gas" | "vibration" | "access" | "system" | "aqi" | "generalAlert";
  message: string;
  timestamp: Date;
  severity: "info" | "alert" | "warning" | "critical";
}

interface AlertsListProps {
  alerts: Alert[];
}

export const AlertsList = ({ alerts }: AlertsListProps) => {
  const getAlertIcon = (type: string) => {
    switch (type) {
      case "gas":
        return AlertTriangle;
      case "vibration":
        return Waves;
      case "access":
        return Activity;
      case "generalAlert":
        return Siren
      default:
        return CheckCircle;
    }
  };

  const getSeverityConfig = (severity: string) => {
    switch (severity) {
      case "critical":
        return { bg: "bg-destructive/20", text: "text-destructive", border: "border-destructive/50" };
      case "warning":
        return { bg: "bg-warning/20", text: "text-warning", border: "border-warning/50" };
      case "alert":
        return { bg: "bg-alert/20", text: "text-alert", border: "border-alert/50" };
      default:
        return { bg: "bg-success/20", text: "text-success", border: "border-success/50" };
    }
  };

  return (
    <Card className="border-border bg-card/50 backdrop-blur-sm">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <AlertTriangle className="h-5 w-5 text-primary" />
          Alertas Recientes
        </CardTitle>
      </CardHeader>
      <CardContent>
        <ScrollArea className="h-80">
          <div className="space-y-3">
            {alerts.map((alert) => {
              const Icon = getAlertIcon(alert.type);
              const config = getSeverityConfig(alert.severity);
              
              return (
                <div
                  key={alert.id}
                  className={`flex items-start gap-3 p-3 rounded-lg border ${config.bg} ${config.border} transition-all hover:scale-[1.02]`}
                >
                  <div className={`mt-0.5 ${config.text}`}>
                    <Icon className="h-5 w-5" />
                  </div>
                  <div className="flex-1 min-w-0">
                    <p className="text-sm font-medium text-foreground">{alert.message}</p>
                    <p className="text-xs text-muted-foreground mt-1">
                      {alert.timestamp.toLocaleTimeString('es-ES')}
                    </p>
                  </div>
                </div>
              );
            })}
          </div>
        </ScrollArea>
      </CardContent>
    </Card>
  );
};
