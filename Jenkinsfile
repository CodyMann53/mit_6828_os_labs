node {
    try {
        stage('Setup') {
            println("Setup stage!")
        }
        stage("Build") {
            println("Build stage!")
            sh 'make'
        }
        stage("Test") {
            println("Test Stage!")
            sh 'make grade'
        }
        stage("Analyze"){
            println("Analyze Stage!")
        }
        if (env.BRANCH_NAME == 'master') {
            stage('Deploy') {
                // Deployment steps for the master branch
                println("Deploying to production!")
            }
        }
        post {
            always {
                sh 'make clean' // Clean up resources
            }
        }
    } catch (Exception e) {
        currentBuild.result = 'FAILURE'
        echo "Error: ${e.message}"
    }
}
