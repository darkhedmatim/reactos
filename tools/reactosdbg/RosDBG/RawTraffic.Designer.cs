﻿namespace RosDBG
{
    partial class RawTraffic
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.RawTrafficText = new System.Windows.Forms.TextBox();
            this.InputLabel = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // RawTrafficText
            // 
            this.RawTrafficText.Dock = System.Windows.Forms.DockStyle.Fill;
            this.RawTrafficText.Location = new System.Drawing.Point(0, 0);
            this.RawTrafficText.MaxLength = 10000000;
            this.RawTrafficText.Multiline = true;
            this.RawTrafficText.Name = "RawTrafficText";
            this.RawTrafficText.ReadOnly = true;
            this.RawTrafficText.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.RawTrafficText.Size = new System.Drawing.Size(413, 289);
            this.RawTrafficText.TabIndex = 0;
            this.RawTrafficText.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.RawTrafficText_KeyPress);
            this.RawTrafficText.MouseUp += new System.Windows.Forms.MouseEventHandler(this.RawTrafficText_MouseUp);
            // 
            // InputLabel
            // 
            this.InputLabel.AutoSize = true;
            this.InputLabel.Location = new System.Drawing.Point(91, 236);
            this.InputLabel.Name = "InputLabel";
            this.InputLabel.Size = new System.Drawing.Size(0, 13);
            this.InputLabel.TabIndex = 1;
            // 
            // RawTraffic
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.InputLabel);
            this.Controls.Add(this.RawTrafficText);
            this.Name = "RawTraffic";
            this.Size = new System.Drawing.Size(413, 289);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox RawTrafficText;
        private System.Windows.Forms.Label InputLabel;
    }
}
