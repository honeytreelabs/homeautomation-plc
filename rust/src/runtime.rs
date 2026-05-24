use crate::gv::Gv;

pub trait Program {
    fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()>;
    fn cycle(&mut self, gv: &mut Gv, now_micros: u64) -> anyhow::Result<()>;
}

pub trait TaskIo {
    fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()>;
    fn before_cycle(&mut self, gv: &mut Gv) -> anyhow::Result<()>;
    fn after_cycle(&mut self, gv: &mut Gv) -> anyhow::Result<()>;
}

pub struct Task {
    pub name: String,
    pub interval_micros: u64,
    programs: Vec<Box<dyn Program>>,
    io: Vec<Box<dyn TaskIo>>,
}

impl Task {
    pub fn new(name: String, interval_micros: u64) -> Self {
        Self {
            name,
            interval_micros,
            programs: Vec::new(),
            io: Vec::new(),
        }
    }

    pub fn add_program(&mut self, program: Box<dyn Program>) {
        self.programs.push(program);
    }

    pub fn add_io(&mut self, io: Box<dyn TaskIo>) {
        self.io.push(io);
    }

    pub fn program_count(&self) -> usize {
        self.programs.len()
    }

    pub fn io_count(&self) -> usize {
        self.io.len()
    }

    pub fn init(&mut self, gv: &mut Gv) -> anyhow::Result<()> {
        for io in &mut self.io {
            io.init(gv)?;
        }

        for program in &mut self.programs {
            program.init(gv)?;
        }

        Ok(())
    }

    pub fn tick(&mut self, gv: &mut Gv, now_micros: u64) -> anyhow::Result<()> {
        for io in &mut self.io {
            io.before_cycle(gv)?;
        }

        for program in &mut self.programs {
            program.cycle(gv, now_micros)?;
        }

        for io in &mut self.io {
            io.after_cycle(gv)?;
        }

        Ok(())
    }
}
