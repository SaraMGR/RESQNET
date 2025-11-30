import { LucideIcon } from "lucide-react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";

interface MetricCardProps {
  title: string;
  value: string | number;
  unit?: string;
  icon: LucideIcon;
  status?: "normal" | "warning" | "critical" | "alert";
  subtitle?: string;
}

export const MetricCard = ({ title, value, unit, icon: Icon, status = "normal", subtitle }: MetricCardProps) => {
  const statusColors = {
    normal: "text-success", 
    warning: "text-warning", 
    alert: "text-alert", // 🎯 Usar la nueva clase 'text-alert'
    critical: "text-destructive", 
  };

  const bgColors = {
    normal: "bg-success/20", 
    warning: "bg-warning/20", 
    alert: "bg-alert/20", // 🎯 Usar la nueva clase 'bg-alert/20'
    critical: "bg-destructive/20",
  };

  const iconColorClass = statusColors[status as keyof typeof statusColors];
  const iconBgClass = bgColors[status as keyof typeof bgColors];

  // 🚨 NUEVA LÓGICA: Determinar si la tarjeta debe parpadear y a qué velocidad
  const isAlertOrCritical = status === "alert" || status === "critical";
  const animationClass = status === "critical" ? "animate-pulse-fast" : (status === "alert" || status === "warning" ? "animate-pulse-slow" : "");

  return (
    <Card className={`border-border bg-card/50 backdrop-blur-sm hover:bg-card/70 transition-all`}>
      <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
        <CardTitle className="text-sm font-medium text-muted-foreground">{title}</CardTitle>
        
        <div className={`h-10 w-10 rounded-lg ${iconBgClass} flex items-center justify-center`}>
          <Icon className={`h-5 w-5 ${iconColorClass}`} />
        </div>
      </CardHeader>
      <CardContent>
        <div className="flex items-baseline gap-2">
          <div className="text-3xl font-bold text-foreground">{value}</div>
          {unit && <span className="text-lg text-muted-foreground">{unit}</span>}
        </div>
        {subtitle && <p className="text-xs text-muted-foreground mt-1">{subtitle}</p>}
      </CardContent>
    </Card>
  );
};
