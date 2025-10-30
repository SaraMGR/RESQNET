import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { UserCheck, Clock } from "lucide-react";
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "@/components/ui/table";
import { Badge } from "@/components/ui/badge";

interface AccessRecord {
  id: number;
  name: string;
  entryTime: Date;
  exitTime: Date | null;
  status: "inside" | "outside";
}

interface AccessLogProps {
  records: AccessRecord[];
}

export const AccessLog = ({ records }: AccessLogProps) => {
  return (
    <Card className="border-border bg-card/50 backdrop-blur-sm">
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <UserCheck className="h-5 w-5 text-primary" />
          Registro de Acceso
        </CardTitle>
      </CardHeader>
      <CardContent>
        <Table>
          <TableHeader>
            <TableRow className="border-border hover:bg-transparent">
              <TableHead className="text-muted-foreground">Nombre</TableHead>
              <TableHead className="text-muted-foreground">Entrada</TableHead>
              <TableHead className="text-muted-foreground">Salida</TableHead>
              <TableHead className="text-muted-foreground">Estado</TableHead>
            </TableRow>
          </TableHeader>
          <TableBody>
            {records.map((record) => (
              <TableRow key={record.id} className="border-border hover:bg-secondary/50">
                <TableCell className="font-medium text-foreground">{record.name}</TableCell>
                <TableCell className="text-muted-foreground">
                  <div className="flex items-center gap-2">
                    <Clock className="h-3 w-3" />
                    {record.entryTime.toLocaleTimeString('es-ES', { hour: '2-digit', minute: '2-digit' })}
                  </div>
                </TableCell>
                <TableCell className="text-muted-foreground">
                  {record.exitTime ? (
                    <div className="flex items-center gap-2">
                      <Clock className="h-3 w-3" />
                      {record.exitTime.toLocaleTimeString('es-ES', { hour: '2-digit', minute: '2-digit' })}
                    </div>
                  ) : (
                    <span className="text-muted-foreground/50">—</span>
                  )}
                </TableCell>
                <TableCell>
                  <Badge
                    variant={record.status === "inside" ? "default" : "secondary"}
                    className={
                      record.status === "inside"
                        ? "bg-success/20 text-success border-success/50"
                        : "bg-muted text-muted-foreground"
                    }
                  >
                    {record.status === "inside" ? "En el salón" : "Fuera"}
                  </Badge>
                </TableCell>
              </TableRow>
            ))}
          </TableBody>
        </Table>
      </CardContent>
    </Card>
  );
};
