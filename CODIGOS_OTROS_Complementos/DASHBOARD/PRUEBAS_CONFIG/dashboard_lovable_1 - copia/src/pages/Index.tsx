import { useState, useEffect } from "react";
// 1. Importar la librería MQTT
import mqtt from 'mqtt';
import { Header } from "@/components/Dashboard/Header";
import { MetricCard } from "@/components/Dashboard/MetricCard";
import { GasLevelMeter } from "@/components/Dashboard/GasLevelMeter";
import { VibrationIndicator } from "@/components/Dashboard/VibrationIndicator";
import { RoomMap } from "@/components/Dashboard/RoomMap";
import { AlertsList } from "@/components/Dashboard/AlertsList";
import { AccessLog } from "@/components/Dashboard/AccessLog";
import { TrendCharts } from "@/components/Dashboard/TrendCharts";
import { Users, Clock, ThermometerSun, Zap, Activity, CloudAlert, Wind, Droplets } from "lucide-react"; // Añadimos Activity para ECO2/Gas
import { Button } from "@/components/ui/button";
import { TvocLevelMeter } from "@/components/Dashboard/TvocLeverMeter";
import { useToast } from "@/components/ui/use-toast"; // <-- ¡Añadir esto!


// --- Constantes de Conexión MQTT ---
const MQTT_BROKER_URL = 'ws://broker.hivemq.com:8000/mqtt'; 

// TÓPICOS RESQNET MQTT
const MQTT_TOPIC_ACCEL = 'resqnet/acelerometro';
const MQTT_TOPIC_AIR = 'resqnet/aire';
const MQTT_TOPIC_NODE_ALERTS = 'resqnet/alertas_nodos'; 
const MQTT_TOPIC_GENERAL_ALERTS = 'resqnet/alertas';
const MQTT_TOPIC_FINGERPRINT_ACCESS = 'resqnet/huella'; // <-- NUEVO TÓPICO

// const MQTT_TOPIC_ACCEL = 'proyecto/acelerometro';
// const MQTT_TOPIC_AIR = 'proyecto/aire';

// const MQTT_TOPIC_NODE_ALERTS = 'proyecto/alertas_nodos'; 
// const MQTT_TOPIC_GENERAL_ALERTS = 'proyecto/alertas';


// --- NUEVOS TIPOS DE PAYLOADS ---
interface DataAcelerometro {
  nodo_id: number;
  x: number;
  y: number;
  z: number;
  prom_xy: number;
}

interface DataAire {
  nodo_id: number;
  temperatura: number;
  humedad: number;
  aqi: number;
  tvoc: number;
  eco2: number;
  prom_eco2: number;
}

// 🟢 NUEVA INTERFAZ PARA ALERTAS DE NODO
interface DataAlertaNodo {
   tipo_evento: string;
   nodo_id: number;
}

// 🟢 NUEVA INTERFAZ PARA PAYLOAD DE HUELLA
interface DataHuella {
    personas_activas: number;
    id_usuario: number;
    estado: "IN" | "OUT";
    nombre: string;
}

interface AccessRecord {
  id: number; // Usaremos id_usuario como id
  name: string; // Nombre o ID de usuario 
  entryTime: Date;
  exitTime: Date | null;
  status: "inside" | "outside";
}

type SystemState = "normal" | "alert" | "critical";
type SensorStatus = "normal" | "warning" | "alert" | "critical";
type AlertType = "gas" | "vibration" | "access" | "system" | "generalAlert";
type AlertSeverity = "info" | "alert" | "warning" | "critical";

const determineSensorStatus = (eco2: number, tvoc: number): SensorStatus => {
    let status: SensorStatus = "normal";

    // 1. Evaluación de eCO2
    if (eco2 > 2000) {
        status = "critical";
    } else if (eco2 > 1000) {
        status = "alert";
    } else if (eco2 > 800) {
        status = "warning";
    }

    // 2. Evaluación de TVOC (Sobrescribe si es más crítico)
    if (tvoc > 600) {
        status = "critical";
    } else if (tvoc > 300 && status !== "critical") {
        status = "alert";
    } else if (tvoc > 100 && status !== "critical" && status !== "alert") {
        status = "warning";
    }
    
    return status;
};

const Index = () => {
  const { toast } = useToast();
  const [systemState, setSystemState] = useState<SystemState>("normal");
  const [eco2Level, setEco2Level] = useState(400); // Base eCO2
  const [tvocLevel, setTvocLevel] = useState(0);
  const [aqiLevel, setAqiLevel] = useState(0);
  const [xValue, setXValue] = useState(0);
  const [yValue, setYValue] = useState(0);
  const [zValue, setZValue] = useState(0);
  const [vibrationValue, setVibrationValue] = useState(0);
  const [temperature, setTemperature] = useState(20);
  const [humidity, setHumidity] = useState(20);
  const [peopleCount, setPeopleCount] = useState(0); // <-- Inicializamos con el número simulado inicial
  // Cambiamos el estado de accessRecords a un array vacío para llenarlo dinámicamente
  const [accessRecords, setAccessRecords] = useState<AccessRecord[]>([]);
  // 💡 NUEVO ESTADO: Para saber qué IDs están actualmente DENTRO y evitar duplicar entradas.
  const [activeUsers, setActiveUsers] = useState<Map<number, AccessRecord>>(new Map());

  // 🟢 ESTADO DE SENSORES: Valores iniciales para que los nodos aparezcan en el mapa
  const [sensors, setSensors] = useState<Array<{
    id: number;
    x: number;
    y: number;
    z: number;
    status: SensorStatus;
    gasLevel: number; // eCO2 del nodo
    tvocLevel: number; // TVOC
    temperatura: number;
    humedad: number;
    aqi: number;
    prom_eco2: number;
    prom_xy: number;
  }>>([
    { id: 1, x: 25, y: 30, z: 0, status: "normal", gasLevel: 400, tvocLevel: 0, temperatura: 0, humedad: 0, aqi: 0, prom_eco2: 400, prom_xy: 0},
    { id: 2, x: 25, y: 30, z: 0, status: "normal", gasLevel: 400, tvocLevel: 0, temperatura: 0, humedad: 0, aqi: 0, prom_eco2: 400, prom_xy: 0},
    { id: 3, x: 25, y: 30, z: 0, status: "normal", gasLevel: 400, tvocLevel: 0, temperatura: 0, humedad: 0, aqi: 0, prom_eco2: 400, prom_xy: 0},
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

  // const [accessRecords] = useState([
  //   { id: 1, name: "Dr. García Martínez", entryTime: new Date(Date.now() - 7200000), exitTime: null, status: "inside" as const },
  //   { id: 2, name: "Ing. Ana López", entryTime: new Date(Date.now() - 6900000), exitTime: null, status: "inside" as const },

  // ]);

  const [trendData, setTrendData] = useState([
    { time: "10:00", gas: 400, vibration: 0.0 },
  ]);

  // --- Lógica de Conexión y Suscripción MQTT con Múltiples Tópicos ---
  useEffect(() => {
    const client = mqtt.connect(MQTT_BROKER_URL);

    client.on('connect', () => {
      console.log('✅ Conectado a MQTT. Suscribiendo a acelerómetro y aire...');
      client.subscribe(MQTT_TOPIC_ACCEL, (err) => { if (err) console.error("Error al suscribirse a Acelerómetro:", err); });
      client.subscribe(MQTT_TOPIC_AIR, (err) => { if (err) console.error("Error al suscribirse a Aire:", err); });
      client.subscribe(MQTT_TOPIC_NODE_ALERTS, (err) => { if (err) console.error("Error al suscribirse a Alertas de Nodos:", err); });
      client.subscribe(MQTT_TOPIC_GENERAL_ALERTS, (err) => { if (err) console.error("Error al suscribirse a Alertas Generales:", err); });
      client.subscribe(MQTT_TOPIC_FINGERPRINT_ACCESS, (err) => { if (err) console.error("Error al suscribirse a Huella:", err); }); // <-- NUEVA SUSCRIPCIÓN
    });

    client.on('message', (topic, message) => {
      try {
          const payload = JSON.parse(message.toString());
          
          if (topic === MQTT_TOPIC_ACCEL) {
              const accelPayload: DataAcelerometro = payload;
              const nodoId = accelPayload.nodo_id;

              const newX = parseFloat(accelPayload.x.toString());
              if (!isNaN(newX)) setXValue(Math.round(newX));

              const newY = parseFloat(accelPayload.y.toString());
              if (!isNaN(newY)) setYValue(Math.round(newY));

              const newZ = parseFloat(accelPayload.z.toString());
              if (!isNaN(newZ)) setZValue(Math.round(newZ));

              const newVibration = parseFloat(accelPayload.prom_xy.toString());
              if (!isNaN(newVibration)) setEco2Level(Math.round(newVibration));
              

              if (!isNaN(newVibration)) {
                  setVibrationValue(Number(newVibration.toFixed(2))); 
                  
                  const now = new Date();
                  const timeStr = `${now.getHours()}:${now.getMinutes().toString().padStart(2, '0')}`;
                  setTrendData((prev) => [
                      ...prev.slice(-19),
                      { time: timeStr, gas: prev[prev.length - 1]?.gas || 400, vibration: Number(newVibration.toFixed(2)) },
                  ]);
              }

              // 2. 🟢 ACTUALIZAR LOS VALORES X, Y, Z EN EL SENSOR PARA EL MAPA
              setSensors(prevSensors => 
                  prevSensors.map(sensor => {
                      if (sensor.id === nodoId) {
                          return { 
                              ...sensor, 
                              x: accelPayload.x,
                              y: accelPayload.y,
                              z: accelPayload.z,
                              prom_xy: accelPayload.prom_xy,
                          };
                      }
                      return sensor;
                  })
              );
              
          } else if (topic === MQTT_TOPIC_AIR) {
              const DataAire: DataAire = payload;
              const nodoId = DataAire.nodo_id;

              // 1. Actualizar métricas principales (promedio/último)
              const newEco2 = parseFloat(DataAire.prom_eco2.toString()); 
              if (!isNaN(newEco2)) setEco2Level(Math.round(newEco2));
              
              const newTvoc = parseFloat(DataAire.tvoc.toString()); 
              if (!isNaN(newTvoc)) setTvocLevel(Math.round(newTvoc));

              const newAqi = parseFloat(DataAire.aqi.toString()); 
              if (!isNaN(newTvoc)) setAqiLevel(Math.round(newAqi));

              const newTemperature = parseFloat(DataAire.temperatura.toString());
              if (!isNaN(newTemperature)) setTemperature(Number(newTemperature.toFixed(1)));

              const newHumidity = parseFloat(DataAire.humedad.toString());
              if (!isNaN(newHumidity)) setHumidity(Number(newHumidity.toFixed(1)));

              // 2. Actualizar estado del sensor individual para RoomMap
              setSensors(prevSensors => 
                  prevSensors.map(sensor => {
                      if (sensor.id === nodoId) {
                          // 🟢 CALCULAR EL STATUS BASADO EN LOS DATOS REALES
                          const newStatus = determineSensorStatus(DataAire.eco2, DataAire.tvoc);
                          return { 
                              ...sensor,
                              status: newStatus, // Actualizado con el status real
                              gasLevel: DataAire.eco2, // eCO2 del nodo para tooltip
                              tvocLevel: DataAire.tvoc, // TVOC del nodo para tooltip
                              temperatura: DataAire.temperatura,
                              humedad: DataAire.humedad,
                              aqi: DataAire.aqi,
                              prom_eco2: DataAire.prom_eco2,
                          };
                      }
                      return sensor;
                  })
              );
              
              // 3. Actualizar la gráfica de tendencia (ECO2)
              const now = new Date();
              const timeStr = `${now.getHours()}:${now.getMinutes().toString().padStart(2, '0')}`;
              setTrendData((prev) => [
                  ...prev.slice(-19),
                  { time: timeStr, gas: Math.round(newEco2), vibration: prev[prev.length - 1]?.vibration || 0 },
              ]);
          
          // 🟢 LÓGICA PARA ALERTAS INDIVIDUALES DE NODO
          } else if (topic === MQTT_TOPIC_NODE_ALERTS) {
            const alertPayload: DataAlertaNodo = payload;
              
              // 1. Determinar la severidad y el tipo basado en el mensaje
              let severity: AlertSeverity = "info";
              let alertType: AlertType = "system";

              // --- LÓGICA DE VIBRACIÓN ---
              if (alertPayload.tipo_evento.includes("RIESGO ESTRUCTURAL")) {
                // 🚨 MÁXIMA SEVERIDAD: RIESGO ESTRUCTURAL
                severity = "critical";
                alertType = "vibration";
              } else if (alertPayload.tipo_evento.includes("TEMBLOR FUERTE")) {
                // ⚠️ SEVERIDAD ALTA: TEMBLOR FUERTE
                severity = "alert"; 
                alertType = "vibration";
              } else if (alertPayload.tipo_evento.includes("TEMBLOR LEVE")) {
                // 🟡 SEVERIDAD MEDIA: TEMBLOR LEVE
                severity = "warning"; 
                alertType = "vibration";
              } 
              
              // --- 💨 LÓGICA DE GAS (NUEVA) ---
               else if (alertPayload.tipo_evento.includes("NIVELES TOXICOS")) {
                // 🚨 CRÍTICA: NIVELES TOXICOS
                severity = "critical";
                alertType = "gas";
              } else if (alertPayload.tipo_evento.includes("VENTILACION URGENTE")) {
                // ⚠️ FUERTE: VENTILACION URGENTE
                severity = "alert";
                alertType = "gas";
              } else if (alertPayload.tipo_evento.includes("CO2 ELEVADO") || alertPayload.tipo_evento.includes("ALERTA LEVE: VENTILAR")) {
                // 🟡 LEVE: CO2 ELEVADO o VENTILAR
                severity = "warning"; 
                alertType = "gas";
              }

              else {
                severity = "info"; 
                alertType = "system";
              }
              
              // 2. 🟢 CONSTRUIR EL MENSAJE REQUERIDO: "Evento en nodo N"
              const message = `${alertPayload.tipo_evento} en nodo ${alertPayload.nodo_id}`;
              
              // 3. Añadir al estado de alertas
              setAlerts(prev => [
                  {
                      id: Date.now(),
                      type: alertType,
                      message: message,
                      timestamp: new Date(),
                      severity: severity,
                  },
                  ...prev.slice(0, 9), // Mantener las últimas 10 alertas
              ]);
          } else if (topic === MQTT_TOPIC_GENERAL_ALERTS) {
            const generalAlertPayload: { evento: string } = payload;

            // Determinar la severidad y tipo para la lista
            let severity: AlertSeverity = "critical"; 
            let alertType: AlertType = "system";

            const message = generalAlertPayload.evento;
            if (message.includes("TEMBLOR")) {
                alertType = "generalAlert";
            } else if (message.includes("TOXICOS") || message.includes("VENTILACION")) {
                alertType = "generalAlert";
            }

            // 1. 📢 MOSTRAR TOAST PARA ALERTA GENERAL (Dura 2 segundos por defecto)
            toast({
                title: "🚨 ¡ALERTA GENERAL CRÍTICA!",
                description: generalAlertPayload.evento,
                variant: "destructive", // Usa el color rojo (destructive) para el impacto visual
                duration: 3500, // Duración de 2 segundos (2000 ms) como solicitaste
            });

            // 2. Añadir a la lista de alertas
            setAlerts(prev => [
                {
                    id: Date.now(),
                    type: alertType,
                    message: `ALERTA GENERAL: ${generalAlertPayload.evento}`,
                    timestamp: new Date(),
                    severity: severity,
                },
                ...prev.slice(0, 9), 
            ]);

          } else if (topic === MQTT_TOPIC_FINGERPRINT_ACCESS) {
              const huellaPayload: DataHuella = payload;
              const { id_usuario, estado, personas_activas, nombre} = huellaPayload;
              const now = new Date();
              
              // 1. Actualizar el conteo total de personas en el salón
              setPeopleCount(personas_activas);

              // 2. Actualizar el registro de acceso
              setActiveUsers(prevActiveUsers => {
                  const newActiveUsers = new Map(prevActiveUsers);
                  const currentRecord = newActiveUsers.get(id_usuario);
                  const userName = nombre;

                  if (estado === "IN") {
                      // Entrada: Crear un nuevo registro
                      if (!currentRecord || currentRecord.status === "outside") {
                          const newRecord: AccessRecord = {
                              id: id_usuario,
                              name: userName,
                              entryTime: now,
                              exitTime: null,
                              status: "inside",
                          };
                          newActiveUsers.set(id_usuario, newRecord);
                          
                          // Añadir el nuevo registro al listado visible (al inicio)
                          setAccessRecords(prevRecords => [newRecord, ...prevRecords.slice(0, 9)]);
                      }
                  } else if (estado === "OUT") {
                      // Salida: Actualizar el registro activo
                      if (currentRecord && currentRecord.status === "inside") {
                          const updatedRecord = { 
                              ...currentRecord,
                              name: currentRecord.name, 
                              exitTime: now, 
                              status: "outside" as const,
                          };
                          newActiveUsers.set(id_usuario, updatedRecord);

                          // Actualizar el registro visible en la lista
                          setAccessRecords(prevRecords => 
                              prevRecords.map(rec => (rec.id === id_usuario && rec.exitTime === null) ? updatedRecord : rec)
                          );
                      }
                  }
                  return newActiveUsers;
              });
          }

      } catch (e) {
        console.error(`Error al procesar mensaje JSON/MQTT del tópico ${topic}:`, e);
      }
    });

    // Limpieza al desmontar el componente
    return () => {
      if (client) {
        client.end();
        console.log('🔌 Desconectado de MQTT.');
      }
    };
  }, []);



  // Las funciones de estado para las MetricCards usan el promedio global de ECO2 y TVOC de MQTT
  const getVibrationIntensity = (): "leve" | "moderada" | "fuerte" => {
    if (vibrationValue < 0.5) return "leve";
    if (vibrationValue < 2) return "moderada";
    return "fuerte";
  };

  const getEco2Status = (): "normal" | "warning" | "alert" | "critical" => {
    if (eco2Level > 2000) return "critical"; 
    if (eco2Level > 1000) return "alert"; 
    if (eco2Level > 800) return "warning";
    return "normal";
  };

  // const getTvocStatus = (): "normal" | "warning" | "alert" | "critical" => {
  //   if (tvocLevel > 600) return "critical"; 
  //   if (tvocLevel > 300) return "alert"; 
  //   if (tvocLevel > 100) return "warning"; 
  //   return "normal";
  // };

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

        {/* <div className="flex justify-between items-center bg-gray-50 dark:bg-gray-800 p-4 rounded-lg shadow-md border border-gray-200 dark:border-gray-700">
          <h1 className="text-2xl font-semibold text-foreground dark:text-white flex items-center">
            RESQNET: Monitoreo del Sistema
          </h1>
          
        
          <div className={`flex items-center gap-2 px-3 py-1.5 rounded-full font-semibold text-sm transition-colors duration-300
            ${systemState === "normal" ? "bg-green-100 text-green-700 dark:bg-green-900 dark:text-green-300" :
              systemState === "alert" ? "bg-yellow-100 text-yellow-700 dark:bg-yellow-900 dark:text-yellow-300" :
              "bg-red-100 text-red-700 dark:bg-red-900 dark:text-red-300"
            }`}
          >
            <div className={`w-3 h-3 rounded-full 
              ${systemState === "normal" ? "bg-green-500" :
                systemState === "alert" ? "bg-yellow-500" :
                "bg-red-500"
              }`}
            />
            Estado: {systemState === "normal" ? "Operación Normal" : systemState === "alert" ? "Alerta Leve" : "CRÍTICO"}
          </div>
        </div> */}

        {/* Quick Metrics */}
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4">

          <MetricCard
            title="Temperatura"
            value={temperature}
            unit="°C"
            icon={ThermometerSun}
            status={temperature > 30 ? "warning" : "normal"}
          />

          <MetricCard
            title="Humedad"
            value={humidity}
            unit="%"
            icon={Droplets}
            status={humidity > 60 ? "warning" : "normal"}
          />
          {/* <MetricCard
            title="Tiempo Activo"
            value="2.5"
            unit="hrs"
            icon={Clock}
            status="normal"
          /> */}
          {/* 🟢 CAMBIO: Restauramos la Metric Card de eCO2 */}
          <MetricCard
            title="eCO2 Promedio"
            value={eco2Level}
            unit="ppm"
            icon={Wind} // Usamos Activity o el icono de Gas que prefieras
            status={getEco2Status()}
          />

          <MetricCard
            title="Personas en Salón"
            value={peopleCount}
            icon={Users}
            status="info"
          />
          
          {/* 🟢 NUEVO: METRIC CARD DE TVOC */}
          {/* <MetricCard
            title="TVOC Promedio"
            value={tvocLevel}
            unit="ppb"
            icon={Activity} // Puedes usar otro icono si prefieres
            status={getTvocStatus()}
          /> */}
        </div>

        {/* Main Monitoring Section */}
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Usamos eco2Level como el "nivel de gas" (CO2) */}
          {/* <GasLevelMeter level={eco2Level} label="Nivel de eCO2" max={2500} />  */}
          
          {/* 🟢 NUEVO: GAS LEVEL METER: TVOC */}
          <TvocLevelMeter 
            level={tvocLevel} 
            label="Nivel de TVOC (ppb)" 
            max={1000} // Valor máximo de TVOC recomendado
          />

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
