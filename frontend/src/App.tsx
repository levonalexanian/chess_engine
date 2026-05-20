import Play from "./pages/Play";

export default function App() {
  return (
    <div className="min-h-screen flex flex-col items-stretch py-6 px-4 bg-slate-50 text-slate-900">
      <header className="w-full max-w-5xl mx-auto flex items-center justify-between mb-6">
        <h1 className="text-xl font-semibold tracking-tight">Chess Engine</h1>
      </header>
      <main className="flex-1 w-full">
        <Play />
      </main>
    </div>
  );
}
