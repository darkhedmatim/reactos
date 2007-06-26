using System;
using System.Drawing;
using System.Collections;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.IO;
using System.Xml.Serialization;
using Microsoft.Win32;


namespace RosTEGUI
{
	public partial class MainForm : Form
    {
        private MainConfig mainConf;
        private Data mainData;

        public MainForm()
        {
            InitializeComponent();
        }

        private void LoadSettingsDialog()
        {
            ListViewItem lvi = VirtMachListView.FocusedItem;

            SettingsForm setFrm = new SettingsForm(lvi.Tag);
            setFrm.StartPosition = FormStartPosition.CenterScreen;
            setFrm.Show();
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            mainData = new Data();
            if (!mainData.LoadMainData())
                MessageBox.Show("Failed to load Main Schema");

            // read config and load any existing vm's
            mainConf = new MainConfig(mainData);

            if (mainConf.LoadMainConfig())
            {
                int num = mainConf.GetNumberOfVms();
                for (int i = 0; i < num; i++)
                {
                    string image = mainConf.GetExistingImage(i);
                    VirtualMachine vm = new VirtualMachine();
                    if (vm.LoadVMConfig(image))
                    {
                        ListViewItem lvi = VirtMachListView.Items.Add(vm.ToString(), 0);
                        lvi.Tag = vm;
                    }
                }
            }
        }

        private void MainMenuHelpAbout_Click(object sender, EventArgs e)
        {
            AboutForm dlg = new AboutForm();
            dlg.StartPosition = FormStartPosition.CenterScreen;
            dlg.Show();
        }

        private void ImageListView_DoubleClick(object sender, EventArgs e)
        {
            LoadSettingsDialog();
        }

        private void toolbarExit_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void CreateNewVirtMach(object sender, EventArgs e)
        {
            NewVMWizard wizFrm = new NewVMWizard();
            wizFrm.StartPosition = FormStartPosition.CenterScreen;

            if (wizFrm.ShowDialog() == DialogResult.OK)
            {
                if (wizFrm.Option == 1)
                {
                    int i = mainConf.AddVirtMach(wizFrm.DefDir);

                    VirtualMachine VirtMach = new VirtualMachine();
                    VirtMach.CreateVMConfig(wizFrm.VMName,
                                            wizFrm.DefDir,
                                            wizFrm.DiskSizeGB,
                                            wizFrm.ExistImg,
                                            wizFrm.MemSizeMB);

                    ListViewItem lvi = VirtMachListView.Items.Add(VirtMach.ToString(), 0);
                    lvi.Tag = VirtMach;
                }
                else if (wizFrm.Option == 2)
                {

                    DirectoryInfo di = Directory.GetParent(wizFrm.ExistImg);
                    int i = mainConf.AddVirtMach(di.FullName);
                    VirtualMachine VirtMach = new VirtualMachine();
                    VirtMach.CreateVMConfig(wizFrm.VMName,
                                            wizFrm.ExistImg,
                                            wizFrm.MemSizeMB);

                    ListViewItem lvi = VirtMachListView.Items.Add(VirtMach.ToString(), 0);
                    lvi.Tag = VirtMach;
                }
                else
                {
                    int i = mainConf.AddVirtMach("Images\\" + wizFrm.VMName);
                    VirtualMachine VirtMach = new VirtualMachine();
                    VirtMach.CreateVMConfig(wizFrm.VMName);

                    ListViewItem lvi = VirtMachListView.Items.Add(VirtMach.ToString(), 0);
                    lvi.Tag = VirtMach;
                }
            }
        }

        private void DeleteVirtMach(object sender, EventArgs e)
        {
            ListViewItem lvi = VirtMachListView.FocusedItem;
            if (lvi != null)
            {
                DeleteVM delFrm = new DeleteVM(lvi.Text/*lvi.Tag*/);
                delFrm.StartPosition = FormStartPosition.CenterScreen;

                if (delFrm.ShowDialog() == DialogResult.OK)
                {
                    if (delFrm.DeleteFullVM)
                    {
                        VirtualMachine vm = (VirtualMachine)lvi.Tag;
                        Directory.Delete(vm.DefDir, true);
                    }

                    mainConf.DeleteVirtMach(lvi.Index/*lvi.Tag*/);
                    VirtMachListView.Items.Remove(lvi);
                }
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            mainConf.SaveMainConfig();

            foreach(ListViewItem lvi in VirtMachListView.Items)
            {
                VirtualMachine vm = (VirtualMachine)lvi.Tag;
                vm.SaveVMConfig();
            }
        }

        private void changeSettingsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            LoadSettingsDialog();
        }
    }
}
