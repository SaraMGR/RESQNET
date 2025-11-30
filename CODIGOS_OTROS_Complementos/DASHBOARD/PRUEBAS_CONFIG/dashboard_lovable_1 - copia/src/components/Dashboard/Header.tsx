import { Activity } from "lucide-react";

export const Header = () => {
  return (
    <header className="border-b border-border bg-card/50 backdrop-blur-sm sticky top-0 z-10">
      <div className="container mx-auto px-6 py-4">
        <div className="flex items-center justify-center gap-4">
          <div className="h-14 w-14 rounded-xl bg-primary/20 flex items-center justify-center">
            <Activity className="h-7 w-7 text-primary" />
          </div>
          <div className="text-center">
            <h1 className="text-3xl font-bold text-foreground tracking-wide">RESQNET</h1>
            <p className="text-sm text-muted-foreground mt-0.5">Sistema de Seguridad</p>
          </div>
        </div>
      </div>
    </header>
  );
};
