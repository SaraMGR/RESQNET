import { useState, useEffect } from "react";
import { Header } from "@/components/Dashboard/Header";
import { MetricCard } from "@/components/Dashboard/MetricCard";
import { GasLevelMeter } from "@/components/Dashboard/GasLevelMeter";
import { VibrationIndicator } from "@/components/Dashboard/VibrationIndicator";
import { RoomMap } from "@/components/Dashboard/RoomMap";
import { AlertsList } from "@/components/Dashboard/AlertsList";
import { AccessLog } from "@/components/Dashboard/AccessLog";
import { TrendCharts } from "@/components/Dashboard/TrendCharts";
import { Users, Clock, ThermometerSun } from "lucide-react";
import { Button } from "@/components/ui/button";

type SystemState = "normal" | "alert" | "critical";

const Index = () => {
  const [systemState, setSystemState] = useState<SystemState>("normal");
  const [gasLevel, setGasLevel] = useState(250);
  const [vibrationValue, setVibrationValue] = useState(0.5);
  const [temperature, setTemperature] = useState(22);
  const [peopleCount, setPeopleCount] = useState(8);

  type SensorStatus = "normal" | "warning" | "alert";
  type AlertType = "gas" | "vibration" | "access" | "system";
  type AlertSeverity = "info" | "warning" | "critical";

  const [sensors, setSensors] = useState<Array<{
    id: number;
    x: number;
    y: number;
    status: SensorStatus;
    gasLevel: number;
  }>>([
    { id: 1, x: 25, y: 30, status: "normal", gasLevel: 180 },
    { id: 2, x: 50, y: 35, status: "normal", gasLevel: 220 },
    { id: 3, x: 75, y: 30, status: "normal", gasLevel: 190 },
    { id: 4, x: 25, y: 70, status: "normal", gasLevel: 240 },
    { id: 5, x: 75, y: 70, status: "normal", gasLevel: 210 },
  ]);

  const [alerts, setAlerts] = useState<Array<{
    id: number;
    type: AlertType;
    message: string;
    timestamp: Date;
    severity: AlertSeverity;
  }>>([
    {
      id: 1,
      type: "system",
      message: "Sistema iniciado correctamente",
      timestamp: new Date(Date.now() - 300000),
      severity: "info",
    },
  ]);

  const [accessRecords] = useState([
    { id: 1, name: "Dr. García Martínez", entryTime: new Date(Date.now() - 7200000), exitTime: null, status: "inside" as const },
    { id: 2, name: "Ing. Ana López", entryTime: new Date(Date.now() - 6900000), exitTime: null, status: "inside" as const },
    { id: 3, name: "Prof. Carlos Ruiz", entryTime: new Date(Date.now() - 5400000), exitTime: new Date(Date.now() - 1800000), status: "outside" as const },
    { id: 4, name: "Téc. María Torres", entryTime: new Date(Date.now() - 4500000), exitTime: null, status: "inside" as const },
  ]);

  const [trendData, setTrendData] = useState([
    { time: "10:00", gas: 200, vibration: 0.3 },
    { time: "10:15", gas: 220, vibration: 0.4 },
    { time: "10:30", gas: 210, vibration: 0.5 },
    { time: "10:45", gas: 240, vibration: 0.6 },
    { time: "11:00", gas: 250, vibration: 0.5 },
  ]);

  // Apply system state scenarios
  useEffect(() => {
    if (systemState === "normal") {
      setGasLevel(200);
      setVibrationValue(0.4);
      setSensors([
        { id: 1, x: 25, y: 30, status: "normal", gasLevel: 180 },
        { id: 2, x: 50, y: 35, status: "normal", gasLevel: 200 },
        { id: 3, x: 75, y: 30, status: "normal", gasLevel: 190 },
        { id: 4, x: 25, y: 70, status: "normal", gasLevel: 195 },
        { id: 5, x: 75, y: 70, status: "normal", gasLevel: 210 },
      ]);
      setAlerts([
        {
          id: Date.now(),
          type: "system",
          message: "Sistema funcionando normalmente",
          timestamp: new Date(),
          severity: "info",
        },
      ]);
    } else if (systemState === "alert") {
      setGasLevel(450);
      setVibrationValue(0.6);
      setSensors([
        { id: 1, x: 25, y: 30, status: "normal", gasLevel: 180 },
        { id: 2, x: 50, y: 35, status: "alert", gasLevel: 650 },
        { id: 3, x: 75, y: 30, status: "normal", gasLevel: 190 },
        { id: 4, x: 25, y: 70, status: "warning", gasLevel: 380 },
        { id: 5, x: 75, y: 70, status: "normal", gasLevel: 210 },
      ]);
      setAlerts([
        {
          id: Date.now(),
          type: "gas",
          message: "Fuga detectada por sensor 2 - Zona central",
          timestamp: new Date(),
          severity: "warning",
        },
        {
          id: Date.now() + 1,
          type: "gas",
          message: "Nivel elevado en sensor 4",
          timestamp: new Date(Date.now() - 30000),
          severity: "warning",
        },
      ]);
    } else if (systemState === "critical") {
      setGasLevel(720);
      setVibrationValue(2.3);
      setSensors([
        { id: 1, x: 25, y: 30, status: "alert", gasLevel: 680 },
        { id: 2, x: 50, y: 35, status: "alert", gasLevel: 750 },
        { id: 3, x: 75, y: 30, status: "warning", gasLevel: 420 },
        { id: 4, x: 25, y: 70, status: "alert", gasLevel: 710 },
        { id: 5, x: 75, y: 70, status: "alert", gasLevel: 690 },
      ]);
      setAlerts([
        {
          id: Date.now(),
          type: "gas",
          message: "¡CRÍTICO! Múltiples sensores detectando fuga severa",
          timestamp: new Date(),
          severity: "critical",
        },
        {
          id: Date.now() + 1,
          type: "vibration",
          message: "Vibración fuerte detectada - Posible sismo",
          timestamp: new Date(Date.now() - 15000),
          severity: "critical",
        },
        {
          id: Date.now() + 2,
          type: "gas",
          message: "Sensor 2: nivel crítico 750 ppm",
          timestamp: new Date(Date.now() - 30000),
          severity: "critical",
        },
        {
          id: Date.now() + 3,
          type: "gas",
          message: "Sensor 4: nivel crítico 710 ppm",
          timestamp: new Date(Date.now() - 45000),
          severity: "critical",
        },
      ]);
    }
  }, [systemState]);

  // Simulate real-time data updates
  useEffect(() => {
    const interval = setInterval(() => {
      // Update gas level
      const newGasLevel = Math.max(150, Math.min(800, gasLevel + (Math.random() - 0.5) * 100));
      setGasLevel(Math.round(newGasLevel));

      // Update vibration
      const newVibration = Math.max(0.1, Math.min(3, vibrationValue + (Math.random() - 0.5) * 0.5));
      setVibrationValue(Number(newVibration.toFixed(2)));

      // Update temperature
      setTemperature(Math.round(22 + Math.random() * 3));

      // Update sensors
      setSensors((prev) =>
        prev.map((sensor) => {
          const newLevel = Math.max(150, Math.min(800, sensor.gasLevel + (Math.random() - 0.5) * 80));
          let status: "normal" | "warning" | "alert" = "normal";
          if (newLevel > 600) status = "alert";
          else if (newLevel > 300) status = "warning";
          
          return { ...sensor, gasLevel: Math.round(newLevel), status };
        })
      );

      // Add random alerts
      if (Math.random() > 0.85) {
        const alertTypes = [
          { type: "gas" as const, message: `Fuga detectada por sensor ${Math.ceil(Math.random() * 5)}`, severity: "warning" as const },
          { type: "vibration" as const, message: "Vibración fuerte detectada en zona sur", severity: "critical" as const },
          { type: "access" as const, message: "Nuevo acceso registrado", severity: "info" as const },
        ];
        const randomAlert = alertTypes[Math.floor(Math.random() * alertTypes.length)];
        
        setAlerts((prev) => [
          {
            id: Date.now(),
            ...randomAlert,
            timestamp: new Date(),
          },
          ...prev.slice(0, 9),
        ]);
      }

      // Update trend data
      const now = new Date();
      const timeStr = `${now.getHours()}:${now.getMinutes().toString().padStart(2, '0')}`;
      setTrendData((prev) => [
        ...prev.slice(-19),
        { time: timeStr, gas: Math.round(newGasLevel), vibration: Number(newVibration.toFixed(1)) },
      ]);
    }, 3000);

    return () => clearInterval(interval);
  }, [gasLevel, vibrationValue]);

  const getVibrationIntensity = (): "leve" | "moderada" | "fuerte" => {
    if (vibrationValue < 1) return "leve";
    if (vibrationValue < 2) return "moderada";
    return "fuerte";
  };

  return (
    <div className="min-h-screen bg-background">
      <Header />
      
      <main className="container mx-auto px-6 py-6 space-y-6">
        {/* System State Simulator */}
        <div className="flex justify-center gap-3 mb-4">
          <Button
            onClick={() => setSystemState("normal")}
            variant={systemState === "normal" ? "default" : "outline"}
            className="min-w-[140px]"
          >
            Estado Normal
          </Button>
          <Button
            onClick={() => setSystemState("alert")}
            variant={systemState === "alert" ? "default" : "outline"}
            className="min-w-[140px]"
          >
            Estado Alerta
          </Button>
          <Button
            onClick={() => setSystemState("critical")}
            variant={systemState === "critical" ? "default" : "outline"}
            className="min-w-[140px]"
          >
            Estado Crítico
          </Button>
        </div>
        {/* Quick Metrics */}
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
          <MetricCard
            title="Personas en Salón"
            value={peopleCount}
            icon={Users}
            status="normal"
          />
          <MetricCard
            title="Temperatura"
            value={temperature}
            unit="°C"
            icon={ThermometerSun}
            status="normal"
          />
          <MetricCard
            title="Tiempo Activo"
            value="2.5"
            unit="hrs"
            icon={Clock}
            status="normal"
          />
          <MetricCard
            title="Gas Promedio"
            value={Math.round(sensors.reduce((acc, s) => acc + s.gasLevel, 0) / sensors.length)}
            unit="ppm"
            icon={Users}
            status={gasLevel > 300 ? "warning" : "normal"}
          />
        </div>

        {/* Main Monitoring Section */}
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          <GasLevelMeter level={gasLevel} />
          <VibrationIndicator intensity={getVibrationIntensity()} value={vibrationValue} />
        </div>

        {/* Room Map and Alerts */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          <div className="lg:col-span-2">
            <RoomMap sensors={sensors} />
          </div>
          <AlertsList alerts={alerts} />
        </div>

        {/* Trends and Access Log */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          <TrendCharts data={trendData} />
          <AccessLog records={accessRecords} />
        </div>
      </main>
    </div>
  );
};

export default Index;
